import gdown
import os
import pathlib

gdown.download(id="1tg_oX3epXoyb4CcHtQMJcbybbo5c2fMm", output="data.7z")
os.system("7z x data.7z -o\"data\"")
pathlib.Path("data.7z").unlink()
