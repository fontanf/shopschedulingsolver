import gdown
import pathlib
import py7zr

# gdown.download(url="https://drive.google.com/file/d/11N58LeLSzC_b6cNwjYTlJ8KlmxFLQ-Ym/view?usp=drive_link", output="data.7z")
gdown.download(id="11N58LeLSzC_b6cNwjYTlJ8KlmxFLQ-Ym", output="data.7z")
with py7zr.SevenZipFile("data.7z", mode="r") as z:
    z.extractall(path="data")
pathlib.Path("data.7z").unlink()
