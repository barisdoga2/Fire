<?php

$PASSWORD = "123";
$ROOT = __DIR__ . "/";

// Load
function loadIndex($file)
{
    if (!file_exists($file)) return [];

    $html = file_get_contents($file);
    preg_match_all('/<p>(.*?)<\/p>/i', $html, $matches);

    $list = [];
    foreach ($matches[1] as $line) {
        $parts = explode(":", $line, 2);
        if (count($parts) == 2)
            $list[$parts[0]] = $parts[1];
    }
    return $list;
}

// Save
function saveIndex($file, $list)
{
    $out  = '
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">

    <!-- Disable caching in all browsers -->
    <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta http-equiv="Expires" content="0" />
    
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
	
	<script src="https://cdnjs.cloudflare.com/ajax/libs/crypto-js/4.2.0/crypto-js.min.js"></script>
	
    <title>FireFS</title>
</head>
<body>
';

    foreach ($list as $p => $md5) {
        $out .= "   <p>{$p}:{$md5}</p>\n";
    }

    $out .= '
</body>
</html>
';
    file_put_contents($file, $out);
}

// Clear
if (isset($_POST['mode']) && $_POST['mode'] === "clear") {

    if (!isset($_POST['pass']) || $_POST['pass'] !== $PASSWORD) {
        http_response_code(403);
        exit("Wrong password!");
    }

    foreach (scandir($ROOT) as $item) {
        if ($item === "." || $item === "..") continue;

        // always keep PHP + HTML
        if (str_ends_with($item, ".php") ||
            str_ends_with($item, ".html"))
            continue;

        $path = $ROOT . $item;

        if (is_dir($path)) {
            // recursive delete directory
            $it = new RecursiveDirectoryIterator($path, RecursiveDirectoryIterator::SKIP_DOTS);
            $files = new RecursiveIteratorIterator($it, RecursiveIteratorIterator::CHILD_FIRST);
            foreach ($files as $file) {
                $file->isDir() ? @rmdir($file->getPathname()) : @unlink($file->getPathname());
            }
            @rmdir($path);
        } else {
            @unlink($path);
        }
    }

    // reset index.html (but keep file)
    saveIndex($ROOT . "index.html", []);

    exit("Cleared!");
}

// Upload
if (isset($_POST['mode']) && $_POST['mode'] === "upload") {

    if (!isset($_POST['pass']) || $_POST['pass'] !== $PASSWORD) {
        http_response_code(403);
        exit("Wrong password!");
    }

    if (!isset($_FILES['file']) || !isset($_POST['relpath'])) {
        http_response_code(400);
        exit("Missing data!");
    }

    $rel = str_replace("\\", "/", $_POST['relpath']);
    $rel = trim($rel, "/");

    $target = $ROOT . $rel;
    $dir = dirname($target);

    if (!is_dir($dir)) mkdir($dir, 0777, true);

    if (!move_uploaded_file($_FILES['file']['tmp_name'], $target)) {
        http_response_code(500);
        exit("Upload failed!");
    }

    /* UPDATE INDEX.HTML */
    $indexFile = $ROOT . "index.html";
    $list = loadIndex($indexFile);

    // compute MD5
    $md5 = md5_file($target);

    // update or insert
    $list[$rel] = $md5;

    saveIndex($indexFile, $list);

    exit("Uploaded!");
}

// Dir
if (isset($_POST['mode']) && $_POST['mode'] === "dir") {

    if (!isset($_POST['pass']) || $_POST['pass'] !== $PASSWORD) {
        http_response_code(403);
        exit("Wrong password!");
    }

    $files = listRecursive($ROOT);

    echo "Directory listing:\n";
    foreach ($files as $f)
        echo $f . "\n";

    exit;
}

function listRecursive($folder)
{
    $result = [];
    foreach (scandir($folder) as $item) {
        if ($item === "." || $item === "..") continue;

        $path = $folder . "/" . $item;
        if (is_dir($path)) {
            $result = array_merge($result, listRecursive($path));
        } else {
            $result[] = substr($path, strlen(__DIR__) + 1);
        }
    }
    return $result;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">

    <!-- Disable caching in all browsers -->
    <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta http-equiv="Expires" content="0" />
    
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
	
    <script src="https://cdnjs.cloudflare.com/ajax/libs/crypto-js/4.2.0/crypto-js.min.js"></script>
	
    <title>FireFS</title>
</head>
<body>

<h2>Fire File Share</h2>

Password:<br>
<input type="password" id="pass"><br><br>

Select a folder to upload:<br>
<input type="file" id="picker" multiple webkitdirectory directory><br><br>

<button onclick="startUpload()">Upload</button>
<button onclick="clearFolder()">Clear</button>
<button onclick="dirList()">Dir</button>

<h3>Result:</h3>
<pre id="result" style="background:#111;color:#0f0;padding:10px;height:250px;overflow:auto;"></pre>

<script>
let fileList = [];

document.getElementById("picker").addEventListener("change", (e) => {
    fileList = [...e.target.files];

    let box = document.getElementById("result");

    box.textContent += "Selected files:\n";
	box.scrollTop = box.scrollHeight;
    for (let f of fileList) {
        box.textContent += "    " + f.webkitRelativePath + "\n";
		box.scrollTop = box.scrollHeight;
    }

    box.scrollTop = box.scrollHeight;
});

async function uploadSingle(file, pass) {
    let fd = new FormData();
    fd.append("mode", "upload");
    fd.append("pass", pass);
    fd.append("file", file);
    fd.append("relpath", file.webkitRelativePath);

    let res = await fetch("FireFS.php", { method: "POST", body: fd });
    return res.text();
}

async function loadServerIndex() {
    let res = await fetch("index.html?" + Date.now());
    let txt = await res.text();

    let map = {}; // path → md5
    let regex = /<p>(.*?)<\/p>/g;
    let m;
    while ((m = regex.exec(txt)) !== null) {
        let parts = m[1].split(":");
        if (parts.length === 2)
            map[parts[0]] = parts[1];
    }
    return map;
}

async function md5File(file) {
    return new Promise((resolve, reject) => {
        let reader = new FileReader();
        reader.onload = () => {
            let wordArray = CryptoJS.lib.WordArray.create(reader.result);
            resolve(CryptoJS.MD5(wordArray).toString());
        };
        reader.onerror = () => reject("File read error");
        reader.readAsArrayBuffer(file);
    });
}

async function startUpload() {
    let pass = document.getElementById("pass").value;
    let box = document.getElementById("result");

    if (!pass) return alert("Enter password.");
    if (fileList.length === 0) {
        box.textContent += "Select files or folders first!\n";
        box.scrollTop = box.scrollHeight;
        return;
    }

    // Load MD5 index from server
    box.textContent += "Loading server index...\n";
    box.scrollTop = box.scrollHeight;

    const serverIndex = await loadServerIndex();
    box.textContent += "Index loaded. Files on server: " + Object.keys(serverIndex).length + "\n";
    box.scrollTop = box.scrollHeight;

    let total = fileList.length;
    let i = 0;

    for (let f of fileList) {
        i++;
        let rel = f.webkitRelativePath;

        box.textContent += `    [${i}/${total}] ${rel}, `;
        box.scrollTop = box.scrollHeight;

        // Local MD5
        let localMD5 = await md5File(f);

        // Compare with server
        if (serverIndex[rel] === localMD5) {
            box.textContent += "Same, Skipped!\n";
            box.scrollTop = box.scrollHeight;
            continue;
        }

        box.textContent += "Updated, Uploading... ";
        box.scrollTop = box.scrollHeight;

        // Upload only if different
        let res = await uploadSingle(f, pass);
        box.textContent += "→ " + res + "\n";
        box.scrollTop = box.scrollHeight;
    }

    box.textContent += "Done.\n";
    box.scrollTop = box.scrollHeight;
}

async function clearFolder() {
    let pass = document.getElementById("pass").value;
    if (!pass) return alert("Enter password.");

    let fd = new FormData();
    fd.append("mode", "clear");
    fd.append("pass", pass);
	
	let box = document.getElementById("result");

    let t = await fetch("FireFS.php", { method: "POST", body: fd }).then(r=>r.text());
    document.getElementById("result").textContent += t + "\n";
    box.scrollTop = box.scrollHeight;
}

async function dirList() {
    let pass = document.getElementById("pass").value;
    if (!pass) return alert("Enter password.");

    let fd = new FormData();
    fd.append("mode", "dir");
    fd.append("pass", pass);
	
	let box = document.getElementById("result");

    let t = await fetch("FireFS.php", { method: "POST", body: fd }).then(r=>r.text());
    document.getElementById("result").textContent += t + "\n";
    box.scrollTop = box.scrollHeight;
}
</script>

</body>
</html>
