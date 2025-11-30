<?php
header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
header("Cache-Control: post-check=0, pre-check=0", false);
header("Pragma: no-cache");
header("Expires: 0");

require __DIR__ . '/config.php';

session_start();

$conn = new mysqli($DB_SERVERNAME, $DB_USERNAME, $DB_PASSWORD, $DB_NAME);
if ($conn->connect_error)
{
    die("Database connection error!");
}

$message = "";
$jwt = "";

function generateRandomString($length = 10) {
    $characters = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    $charactersLength = strlen($characters);
    $randomString = '';

    for ($i = 0; $i < $length; $i++) {
        $randomString .= $characters[random_int(0, $charactersLength - 1)];
    }

    return $randomString;
}

if ($_SERVER["REQUEST_METHOD"] === "POST" && isset($_POST['login']))
{
    $user = trim($_POST['username'] ?? '');
    $pass = trim($_POST['password'] ?? '');
    $hash = hash('sha256', $pass);

    $stmt = $conn->prepare("SELECT * FROM users WHERE username=? AND password=?");
    $stmt->bind_param("ss", $user, $hash);
    $stmt->execute();
    $result = $stmt->get_result();
	$row = mysqli_fetch_assoc($result);
    if ($result->num_rows > 0)
	{
        $_SESSION['username'] = $row["username"];
        $_SESSION['id'] = $row["id"];
		
		$stmt = $conn->prepare("SELECT * FROM sessions WHERE user_id=?");
		$stmt->bind_param("i", $row["id"]);
		$stmt->execute();
		$result = $stmt->get_result();
		$row = mysqli_fetch_assoc($result);
		if ($result->num_rows == 0)
		{
			$stmt = $conn->prepare("INSERT INTO sessions (user_id, session_key, valid_until) VALUES (?, ?, ?)");
			$newValid = date("Y-m-d H:i:s", time() + 3600); // convert to datetime string
			$rngStr = generateRandomString(16);
			$stmt->bind_param("isi", $_SESSION['id'], $rngStr, $newValid);
			$stmt->execute();
			
			$stmt = $conn->prepare("SELECT * FROM sessions WHERE user_id=?");
			$stmt->bind_param("i", $_SESSION['id']);
			$stmt->execute();
			$result = $stmt->get_result();
			$row = mysqli_fetch_assoc($result);
		}
		
		if ($result->num_rows > 0) {
			$newValid = date("Y-m-d H:i:s", time() + 3600); // convert to datetime string
			
			// if the session will expire within the next hour, renew it
			if (strtotime($row["valid_until"]) <= time()) {
				$newKey = generateRandomString(16);
				$newValid = date("Y-m-d H:i:s", time() + 3600); // convert to datetime string

				$stmt = $conn->prepare("UPDATE sessions SET session_key = ?, valid_until = ? WHERE user_id = ?");
				$stmt->bind_param("ssi", $newKey, $newValid, $_SESSION['id']); // fixed: s = string, i = int
				$stmt->execute();

				$stmt = $conn->prepare("SELECT * FROM sessions WHERE user_id = ?");
				$stmt->bind_param("i", $_SESSION['id']);
				$stmt->execute();
				$result = $stmt->get_result();
				$row = mysqli_fetch_assoc($result);
			}

			$message .= "Login successfull!";
			$jwt = $row["id"] . ":" . $_SESSION['id'] . ":" . $row["session_key"];
			$res = 1;
		}
		else
		{
			$message = "Session not found!";
			$res = 0;
		}
    }
	else
	{
        $message = "Wrong username or password!";
		$res = 0;
    }
}

if ($_SERVER["REQUEST_METHOD"] === "POST" && isset($_POST['register']))
{
    $user = trim($_POST['username'] ?? '');
    $pass = trim($_POST['password'] ?? '');
    if ($user && $pass)
    {
        $hash = hash('sha256', $pass);
        $stmt = $conn->prepare("INSERT INTO users (username, password) VALUES (?, ?)");
        try
        {
            $stmt->bind_param("ss", $user, $hash);
            $stmt->execute();
            $newUserId = $stmt->insert_id;
            $stmt2 = $conn->prepare("INSERT INTO user_stats (user_id) VALUES (?)");
            $stmt2->bind_param("i", $newUserId);
            $stmt2->execute();
            $stmt2->close();
            $message = "Kayit basarili.";
        }
        catch (mysqli_sql_exception $e)
		{
            if (str_contains($e->getMessage(), 'Duplicate entry'))
			{
                $message = "Username already exists!";
            }
			else
			{
                $message = "Illegal characters found!";
            }
        }
        $stmt->close();
    }
    else
    {
        $message = "Empty username and/or password!";
    }
}

if (isset($_GET['code']) && isset($_GET['state']))
{
    $token_url = 'https://oauth2.googleapis.com/token';
    $post_fields = ['code' => $_GET['code'], 'client_id' => $GOOGLE_CLIENT_ID, 'client_secret' => $GOOGLE_CLIENT_SECRET, 'redirect_uri' => $GOOGLE_REDIRECT_URI, 'grant_type' => 'authorization_code'];

    $ch = curl_init($token_url);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $post_fields);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    $response = curl_exec($ch);
    curl_close($ch);

    $token_data = json_decode($response, true);

    if (isset($token_data['id_token']))
	{
		$parts = explode('.', $token_data['id_token']);
		$payload = $parts[1] ?? '';
        $user_data = json_decode(base64_decode(str_replace(['-', '_'], ['+', '/'], $payload)), true);

        $_SESSION['username'] = $user_data['email'] ?? 'GoogleUser';
        $message = "Login successfull!";
		$res = 1;
    }
	else
	{
        $message = "Failed to login with Google!";
		$res = 0;
    }
}

$google_login_url = (function() use ($GOOGLE_CLIENT_ID, $GOOGLE_REDIRECT_URI)
{
    $scope = urlencode('openid email profile');
    $state = bin2hex(random_bytes(8));
    $_SESSION['oauth2state'] = $state;
    return "https://accounts.google.com/o/oauth2/v2/auth?response_type=code&client_id={$GOOGLE_CLIENT_ID}&redirect_uri=" . urlencode($GOOGLE_REDIRECT_URI) . "&scope={$scope}&state={$state}&access_type=online&prompt=select_account";
})();

$conn->close();

?>

<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>FireWeb Login / Register</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 60px; }
    input { padding: 8px; margin: 5px; width: 200px; }
    button { padding: 8px 15px; margin: 5px; }
    .msg { margin-top: 20px; font-weight: bold; }
    textarea { width: 80%; height: 100px; margin-top: 10px; }
  </style>
</head>
<body>
  <h1><a href="Fire.7z">Download Client</a></h1>
  <br>
  <br>
  <br>
  <h2>FireWeb Login / Register</h2>
  <form method="POST" action="index.php">
    <input type="text" name="username" placeholder="Username" required><br>
    <input type="password" name="password" placeholder="Password" required><br>
    <button type="submit" name="login">Login</button>
    <button type="submit" name="register">Register</button>
  </form>

  <br>
  <a href="<?= htmlspecialchars($google_login_url) ?>">
    <button>Login with Google</button>
  </a>

  <div class="msg"><?= htmlspecialchars($message) ?></div>

  <?php if ($jwt): ?>
    <h3>JWT Token</h3>
    <textarea name="jwt" readonly><?= $jwt ?></textarea>
  <?php endif; ?>

</body>
</html>
