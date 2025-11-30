import gdown
import pathlib
import py7zr

gdown.download(id="1eyx5z4bJMAab28xKNy302ieYU2vzTaH7", output="data.7z")
with py7zr.SevenZipFile("data.7z", mode="r") as z:
    z.extractall(path="data")
pathlib.Path("data.7z").unlink()
