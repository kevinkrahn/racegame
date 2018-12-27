#!/usr/bin/python3

import os, sys, subprocess, time
import utils
from fetch_dependencies import *

start_time = time.time()

downloadAndBuildDependencies()

build_type = 'debug'
job_count = str(os.cpu_count())

utils.ensureDir(os.path.join(utils.getRepoPath(), 'bin'))

returncode = False
if sys.platform == 'win32':
    build_dir = os.path.join(utils.getRepoPath(), 'build')
    if not os.path.isdir(build_dir):
        ensureDir(build_dir)
        subprocess.run(['cmake', utils.getRepoPath()], cwd=build_dir)
    returncode = subprocess.run(['cmake', '--build', '--target', build_type, '--parallel', job_count]).returncode
else:
    build_dir = os.path.join(utils.getRepoPath(), 'build', build_type)
    if not os.path.isdir(build_dir):
        ensureDir(build_dir)
        subprocess.run(['cmake', utils.getRepoPath(), '-DCMAKE_BUILD_TYPE='+build_type, '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'], cwd=build_dir)
    returncode = subprocess.run(['cmake', '--build', build_dir, '--parallel', job_count]).returncode

print('Took {:.2f} seconds'.format(time.time() - start_time))

if returncode == 0:
    subprocess.run([os.path.join(utils.getRepoPath(), 'bin', 'game')], cwd=os.path.join(utils.getRepoPath(), 'bin'))
