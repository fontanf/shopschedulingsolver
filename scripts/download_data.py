import gdown
import pathlib
import py7zr

gdown.download(id="1e2QCelLv6P5DTv9jKm4Hnmf4-crdSAZl", output="data.7z")
with py7zr.SevenZipFile("data.7z", mode="r") as z:
    z.extractall(path="data")
pathlib.Path("data.7z").unlink()
