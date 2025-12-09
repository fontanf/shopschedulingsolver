import gdown
import pathlib
import py7zr

gdown.download(id="1blCYSmH_Z4XKZJlY9-8xJpVfDZk0UiDo", output="data.7z")
with py7zr.SevenZipFile("data.7z", mode="r") as z:
    z.extractall(path="data")
pathlib.Path("data.7z").unlink()
