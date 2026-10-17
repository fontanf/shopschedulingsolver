"""Create the data archives listed in data/data.json from the content of
data/.

To publish a new version of an archive:
1. Run this script; it prints the SHA-256 checksum of each archive.
2. Upload the archive to a new GitHub release, e.g.:
   gh release create data-tests-v2 data/tests.tar.xz --title "Test data v2"
   Never replace the asset of an existing release: older versions of the
   repository still download it.
3. Update the URL and the checksum of the archive in data/data.json.
"""

import argparse
import hashlib
import json
import pathlib
import tarfile


DATA_DIRECTORY = pathlib.Path("data")
MANIFEST_PATH = DATA_DIRECTORY / "data.json"


def sha256sum(path):
    hasher = hashlib.sha256()
    with open(path, "rb") as file:
        for chunk in iter(lambda: file.read(1 << 20), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


parser = argparse.ArgumentParser(description="Create the data archives.")
parser.add_argument(
        "--archives",
        type=str,
        nargs="+",
        help="names of the archives to create (default: all)")
args = parser.parse_args()

with open(MANIFEST_PATH, "r") as manifest_file:
    manifest = json.load(manifest_file)

for archive in manifest["archives"]:
    name = archive["name"]
    if args.archives is not None and name not in args.archives:
        continue
    exclude_suffixes = tuple(archive.get("exclude_suffixes", []))

    def exclude(tar_info):
        if tar_info.isfile() and tar_info.name.endswith(exclude_suffixes):
            return None
        # Make the archive independent of the local user.
        tar_info.uid = tar_info.gid = 0
        tar_info.uname = tar_info.gname = ""
        return tar_info

    archive_path = DATA_DIRECTORY / pathlib.Path(archive["url"]).name
    print(f"{name}: creating {archive_path}")
    with tarfile.open(archive_path, "w:xz", dereference=True) as tar:
        for directory in sorted(archive["directories"]):
            tar.add(DATA_DIRECTORY / directory, arcname=directory, filter=exclude)
    print(f"{name}: sha256 {sha256sum(archive_path)}")
