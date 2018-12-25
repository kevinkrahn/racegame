#!/usr/bin/python3

import os, sys, subprocess, time
from fetch_dependencies import *

start_time = time.time()

downloadAndBuildDependencies()

build_type = 'debug'
job_count = str(os.cpu_count())

ensureDir(os.path.join(getRepoPath(), 'bin'))

if sys.platform == 'win32':
    build_dir = os.path.join(getRepoPath(), 'build')
    if not os.path.isdir(build_dir):
        ensureDir(build_dir)
        subprocess.run(['cmake', getRepoPath()], cwd=build_dir)
    subprocess.run(['cmake', '--build', '--target', build_type, '--parallel', job_count])
else:
    build_dir = os.path.join(getRepoPath(), 'build', build_type)
    if not os.path.isdir(build_dir):
        ensureDir(build_dir)
        subprocess.run(['cmake', getRepoPath(), '-DCMAKE_BUILD_TYPE='+build_type, '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'], cwd=build_dir)
    subprocess.run(['cmake', '--build', build_dir, '--parallel', job_count])

print('Took {:.2f} seconds'.format(time.time() - start_time))
