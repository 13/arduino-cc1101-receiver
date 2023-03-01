#!/usr/bin/env python

from datetime import datetime
import subprocess

now = datetime.now().strftime("%d.%m.%Y %H:%M:%S")
rev = (
    subprocess.check_output(["git", "describe", "--abbrev=7", "--always", "--tags"])
    .strip()
    .decode("utf-8")
)
rev_short = (
    subprocess.check_output(["git", "describe", "--abbrev=0", "--always", "--tags"])
    .strip()
    .decode("utf-8")
)
ret = rev + ' ' + now
print("'-DGIT_VERSION=\"%s\"'" % ret)
print("'-DGIT_VERSION_SHORT=\"%s\"'" % rev_short)