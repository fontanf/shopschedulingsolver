import gdown
import pathlib
import py7zr

gdown.download(id="1tg_oX3epXoyb4CcHtQMJcbybbo5c2fMm", output="data.7z")
with py7zr.SevenZipFile("data.7z", mode="r") as z:
    z.extractall(path="data")
pathlib.Path("data.7z").unlink()
