#!/usr/bin/python3

import os, sys, subprocess, glob, shutil
import utils

blenderCommand = 'blender' if sys.platform != 'win32' else '"C:/Program Files/Blender Foundation/Blender/blender.exe"'

def exportFile(blendFile):
    output = subprocess.check_output([blenderCommand, '-b', blendFile, '-P', 'blender_exporter.py', '--enable-autoexec']).decode('utf-8')
    return 'Saved to file:' in output

def copyFile(file):
    shutil.copy2(file, os.path.join(utils.getRepoPath(), 'bin'))

for file in glob.glob(os.path.join(utils.getRepoPath(), 'assets/**/*.blend')):
    exportFile(file)

utils.ensureDir(os.path.join(utils.getRepoPath(), 'bin'))
for file in glob.glob(os.path.join(utils.getRepoPath(), 'assets/**/*')):
    if not file.endswith(".blend"):
        copyFile(file)

