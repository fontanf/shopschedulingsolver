"""Download the data archives listed in data/data.json and extract them in
data/.

The manifest pins the URL and the SHA-256 checksum of each archive, so each
version of the repository downloads the data it has been tested with. An
archive is only downloaded again when its checksum in the manifest changes.
"""

import argparse
import hashlib
import json
import os
import pathlib
import shutil
import tarfile
import urllib.request


DATA_DIRECTORY = pathlib.Path("data")
MANIFEST_PATH = DATA_DIRECTORY / "data.json"


def sha256sum(path):
    hasher = hashlib.sha256()
    with open(path, "rb") as file:
        for chunk in iter(lambda: file.read(1 << 20), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def safe_extract(archive_path, destination):
    """Extract a tar archive, refusing members that would be written outside
    of the destination directory."""
    destination = destination.resolve()
    with tarfile.open(archive_path, "r:*") as archive:
        for member in archive.getmembers():
            if not (member.isfile() or member.isdir()):
                raise ValueError(f"Unexpected member type: {member.name}")
            member_path = (destination / member.name).resolve()
            if destination not in member_path.parents and member_path != destination:
                raise ValueError(f"Unsafe member path: {member.name}")
        archive.extractall(destination)


parser = argparse.ArgumentParser(description="Download the data archives.")
parser.add_argument(
        "--archives",
        type=str,
        nargs="+",
        help="names of the archives to download (default: all)")
args = parser.parse_args()

with open(MANIFEST_PATH, "r") as manifest_file:
    manifest = json.load(manifest_file)

for archive in manifest["archives"]:
    name = archive["name"]
    if args.archives is not None and name not in args.archives:
        continue

    # The checksum of the last extracted version of the archive is stored in
    # a marker file.
    marker_path = DATA_DIRECTORY / f".{name}.sha256"
    if marker_path.exists() and marker_path.read_text().strip() == archive["sha256"]:
        print(f"{name}: up to date")
        continue

    archive_path = DATA_DIRECTORY / pathlib.Path(archive["url"]).name
    print(f"{name}: downloading {archive['url']}")
    with urllib.request.urlopen(archive["url"]) as response, open(archive_path, "wb") as file:
        shutil.copyfileobj(response, file)

    checksum = sha256sum(archive_path)
    if checksum != archive["sha256"]:
        archive_path.unlink()
        raise ValueError(
                f"{name}: wrong checksum; "
                f"expected {archive['sha256']}; "
                f"got {checksum}.")

    print(f"{name}: extracting")
    safe_extract(archive_path, DATA_DIRECTORY)
    archive_path.unlink()
    marker_path.write_text(archive["sha256"] + "\n")
