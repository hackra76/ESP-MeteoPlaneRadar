import subprocess
import urllib.request
import urllib.error
import json
import os
import sys

def get_github_token():
    proc = subprocess.run(
        ["git", "credential", "fill"],
        input="protocol=https\nhost=github.com\n",
        capture_output=True,
        text=True,
        check=True
    )
    for line in proc.stdout.splitlines():
        if line.startswith("password="):
            return line.split("=", 1)[1].strip()
    return None

def main():
    token = get_github_token()
    if not token:
        print("Error: Could not retrieve GitHub token from git credential manager")
        sys.exit(1)

    owner = "hackra76"
    repo = "ESP-MeteoPlaneRadar"
    tag_name = "v2.0.0"
    release_title = "v2.0.0: Animated Pixel Art DigiCat & Autonomous AI Companion"
    
    notes_path = os.path.join("release", "release_notes_v2.0.0.md")
    with open(notes_path, "r", encoding="utf-8") as f:
        release_notes = f.read()

    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "Release-Publisher"
    }

    # 1. Create Release
    payload = {
        "tag_name": tag_name,
        "name": release_title,
        "body": release_notes,
        "draft": False,
        "prerelease": False
    }

    create_url = f"https://api.github.com/repos/{owner}/{repo}/releases"
    req = urllib.request.Request(
        create_url,
        data=json.dumps(payload).encode("utf-8"),
        headers={**headers, "Content-Type": "application/json"},
        method="POST"
    )

    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            release_id = data["id"]
            upload_url_template = data["upload_url"]
            print(f"Created release ID: {release_id}")
            print(f"Release HTML URL: {data['html_url']}")
    except urllib.error.HTTPError as e:
        err_msg = e.read().decode('utf-8')
        print(f"Failed to create release: {e.code} {e.reason} - {err_msg}")
        if "already_exists" in err_msg:
            # Fetch existing release
            get_req = urllib.request.Request(
                f"https://api.github.com/repos/{owner}/{repo}/releases/tags/{tag_name}",
                headers=headers
            )
            with urllib.request.urlopen(get_req) as resp:
                data = json.loads(resp.read().decode("utf-8"))
                release_id = data["id"]
                upload_url_template = data["upload_url"]
                print(f"Found existing release ID: {release_id}")
        else:
            sys.exit(1)

    # Base upload URL: remove '{?name,label}'
    base_upload_url = upload_url_template.split("{")[0]

    assets = [
        os.path.join("release", "MeteoPlaneRadar-v2.0.0-factory.bin"),
        os.path.join("release", "MeteoPlaneRadar-v2.0.0-ota.bin"),
        os.path.join("release", "bootloader.bin"),
        os.path.join("release", "partitions.bin")
    ]

    for asset_path in assets:
        if not os.path.isfile(asset_path):
            print(f"Asset not found: {asset_path}")
            continue
        
        filename = os.path.basename(asset_path)
        upload_url = f"{base_upload_url}?name={filename}"
        print(f"Uploading {filename} ({os.path.getsize(asset_path)} bytes)...")
        
        with open(asset_path, "rb") as f:
            file_data = f.read()

        upload_req = urllib.request.Request(
            upload_url,
            data=file_data,
            headers={**headers, "Content-Type": "application/octet-stream"},
            method="POST"
        )
        try:
            with urllib.request.urlopen(upload_req) as resp:
                print(f"Uploaded {filename} successfully ({resp.status}).")
        except urllib.error.HTTPError as e:
            err_msg = e.read().decode('utf-8')
            print(f"Failed to upload {filename}: {e.code} - {err_msg}")

    print("All release assets processed successfully.")

if __name__ == "__main__":
    main()
