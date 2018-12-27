#!/usr/bin/python3

import os, sys, subprocess, glob, shutil, pickle, errno
import utils

timestampCacheFilename = os.path.join(utils.getRepoPath(), 'build', 'asset_timestamps')
timestampCache = {}

if not 'force' in sys.argv:
    try:
        with open(timestampCacheFilename, 'rb') as file:
            timestampCache = pickle.load(file)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise

def exportFile(blendFile):
    blenderCommand = 'blender' if sys.platform != 'win32' else '"C:/Program Files/Blender Foundation/Blender/blender.exe"'
    output = subprocess.check_output([blenderCommand, '-b', blendFile, '-P', 'blender_exporter.py', '--enable-autoexec']).decode('utf-8')
    return 'Saved to file:' in output

def copyFile(file):
    shutil.copy2(file, os.path.join(utils.getRepoPath(), 'bin'))

utils.ensureDir(os.path.join(utils.getRepoPath(), 'bin'))

modified = False
for file in glob.glob(os.path.join(utils.getRepoPath(), 'assets/**/*'), recursive=True):
    fullpath = os.path.abspath(file)
    timestamp = os.path.getmtime(fullpath)
    print(fullpath, ':', timestamp)
    if timestampCache.get(fullpath, 0) < timestamp:
        if file.endswith(".blend"):
            print('Exporting', os.path.basename(file))
            if exportFile(file):
                modified = True
                timestampCache[fullpath] = timestamp
        else:
            print('Copying', os.path.basename(file))
            copyFile(file)
            modified = True
            timestampCache[fullpath] = timestamp

if modified:
    with open(timestampCacheFilename, 'wb') as file:
        pickle.dump(timestampCache, file)
else:
    print('No assets to save')

