import hashlib
import os

ROOT = "web_api"
OUT = os.path.join(ROOT, "index.html")

def md5_file(path):
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()

entries = []

for root, dirs, files in os.walk(ROOT):
    for name in files:
        full = os.path.join(root, name)
        if os.path.abspath(full) == os.path.abspath(OUT):
            continue

        rel = os.path.relpath(full, ROOT).replace("\\", "/")
        try:
            digest = md5_file(full)
            entries.append((rel, digest))
        except Exception:
            pass

with open(OUT, "w", encoding="utf-8") as f:
    f.write("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta http-equiv="Expires" content="0" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/crypto-js/4.2.0/crypto-js.min.js"></script>
    <title>FireFS</title>
</head>
<body>
""")
    for path, md5 in sorted(entries):
        f.write(f"   <p>{path}:{md5}</p>\n")

    f.write("""
</body>
</html>
""")

print(f"index.html generated with {len(entries)} entries")
