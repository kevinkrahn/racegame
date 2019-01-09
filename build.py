#!/usr/bin/python3

import os, sys, subprocess, urllib.request, zipfile, tempfile, errno, pickle, glob, time, shutil
import xml.etree.ElementTree as ET
from argparse import ArgumentParser

repoPath = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).decode('utf-8').strip()

def ensureDir(dir):
    try:
        os.makedirs(dir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

def fetch(url, name):
    resultPath = os.path.join('external', name)
    ensureDir('external')

    if '.zip' in url:
        if os.path.isdir(resultPath):
            return False
        else:
            tmpPath = tempfile.mktemp()
            urllib.request.urlretrieve(url, tmpPath)
            tmpResultPath = os.path.join('external', 'unzipped')
            ensureDir(tmpResultPath)
            zipfile.ZipFile(tmpPath, 'r').extractall(tmpResultPath)
            contents = os.listdir(tmpResultPath)
            if len(contents) == 1 and os.path.isdir(os.path.join(tmpResultPath, contents[0])):
                os.rename(os.path.join(tmpResultPath, contents[0]), resultPath)
                shutil.rmtree(tmpResultPath)
            else:
                os.rename(tmpResultPath, resultPath)

            return True
    else:
        if os.path.isdir(resultPath):
            #subprocess.run(['git', 'pull'], cwd=resultPath)
            return False
        else:
            subprocess.run(['git', 'clone', url, resultPath])
            return True

def fetch_dependencies():
    # STB
    fetch('https://github.com/nothings/stb.git', 'stb')

    # GLM
    fetch('https://github.com/g-truc/glm', 'glm')

    # PhysX
    if fetch('https://github.com/NVIDIAGameWorks/PhysX', 'physx'):
        physxdir = os.path.join('external', 'physx', 'physx')
        if sys.platform == 'win32':
            xmlFile = os.path.join(physxdir, 'buildtools', 'presets', 'public', 'vc15win64.xml')

            tree = ET.parse(xmlFile)
            for cmakeSwitch in tree.getroot().iter('cmakeSwitch'):
                if cmakeSwitch.get('name') in ['PX_BUILDSNIPPETS', 'PX_BUILDPUBLICSAMPLES', 'NV_USE_STATIC_WINCRT', 'PX_GENERATE_STATIC_LIBRARIES']:
                    cmakeSwitch.set('value', 'False')
                if cmakeSwitch.get('name') in ['NV_USE_DEBUG_WINCRT']:
                    cmakeSwitch.set('value', 'True')
            tree.write(xmlFile)

            subprocess.run([os.path.abspath(os.path.join(physxdir, 'generate_projects.bat')), 'vc15win64'], cwd=physxdir)
            subprocess.run(['cmake', '--build', os.path.abspath(os.path.join(physxdir, 'compiler', 'vc15win64')),
                '--parallel', str(os.cpu_count()), '--config', 'debug'], cwd=physxdir)
            subprocess.run(['cmake', '--build', os.path.abspath(os.path.join(physxdir, 'compiler', 'vc15win64')),
                '--parallel', str(os.cpu_count()), '--config', 'release'], cwd=physxdir)
        else:
            subprocess.run([os.path.join(physxdir, 'generate_projects.sh'), 'linux'], cwd=physxdir)
            subprocess.run(['cmake', '--build', os.path.join(physxdir, 'compiler', 'linux-checked'),
                '--parallel', str(os.cpu_count())], cwd=physxdir)
            subprocess.run(['cmake', '--build', os.path.join(physxdir, 'compiler', 'linux-release'),
                '--parallel', str(os.cpu_count())], cwd=physxdir)

    # SDL2
    if sys.platform == 'win32':
        if fetch('https://www.libsdl.org/release/SDL2-devel-2.0.9-VC.zip', 'sdl2'):
            os.rename(os.path.join('external/sdl2/include'), 'external/sdl2/SDL2')


def build_assets(force):
    timestampCacheFilename = os.path.join('build', 'asset_timestamps')
    timestampCache = {}

    if not force:
        try:
            with open(timestampCacheFilename, 'rb') as file:
                timestampCache = pickle.load(file)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise

    def exportFile(blendFile):
        blenderCommand = 'blender' if sys.platform != 'win32' else 'C:/Program Files/Blender Foundation/Blender/blender.exe'
        output = subprocess.check_output([blenderCommand, '-b', blendFile, '-P', 'blender_exporter.py', '--enable-autoexec']).decode('utf-8')
        return 'Saved to file:' in output

    def copyFile(file):
        dest = os.path.join('bin', os.path.relpath(file, 'assets'))
        ensureDir(os.path.dirname(dest))
        shutil.copy2(file, dest)

    modified = False
    for file in glob.glob('assets/**/*', recursive=True):
        if not os.path.isfile(file):
            continue

        timestamp = os.path.getmtime(file)
        #print(file, ':', timestamp)
        if timestampCache.get(file, 0) < timestamp:
            if file.endswith(".blend"):
                print('Exporting', file)
                if exportFile(file):
                    modified = True
                    timestampCache[file] = timestamp
            else:
                print('Copying', file)
                copyFile(file)
                modified = True
                timestampCache[file] = timestamp

    if modified:
        ensureDir('build')
        with open(timestampCacheFilename, 'wb') as file:
            pickle.dump(timestampCache, file)
    else:
        print('No assets to save')

def build():
    build_type = 'debug'
    job_count = str(os.cpu_count())

    ensureDir('bin')

    returncode = False
    if sys.platform == 'win32':
        build_dir = 'build'
        ensureDir(build_dir)
        if not os.path.isfile(os.path.join(build_dir, 'CMakeCache.txt')):
            if subprocess.run(['cmake', '-DCMAKE_GENERATOR_PLATFORM=x64', repoPath], cwd=build_dir).returncode != 0:
                return False
        if subprocess.run(['cmake', '--build', build_dir, '--config', build_type, '--parallel', job_count]).returncode != 0:
            return False
        if not os.path.isfile(os.path.join('bin', 'SDL2.dll')):
            shutil.copy2(os.path.join('external', 'sdl2', 'lib', 'x64', 'SDL2.dll'), 'bin')

        dllDir = 'debug' if build_type == 'debug' else 'release'
        shutil.copy2(os.path.join('external', 'physx', 'physx', 'bin', 'win.x86_64.vc141.md', dllDir, 'PhysXCommon_64.dll'), 'bin')
        shutil.copy2(os.path.join('external', 'physx', 'physx', 'bin', 'win.x86_64.vc141.md', dllDir, 'PhysXCooking_64.dll'), 'bin')
        shutil.copy2(os.path.join('external', 'physx', 'physx', 'bin', 'win.x86_64.vc141.md', dllDir, 'PhysXFoundation_64.dll'), 'bin')
        shutil.copy2(os.path.join('external', 'physx', 'physx', 'bin', 'win.x86_64.vc141.md', dllDir, 'PhysX_64.dll'), 'bin')
    else:
        build_dir = os.path.join('build', build_type)
        if not os.path.isdir(build_dir):
            ensureDir(build_dir)
            if subprocess.run(['cmake', repoPath, '-DCMAKE_BUILD_TYPE='+build_type, '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'], cwd=build_dir).returncode != 0:
                return False
        if subprocess.run(['cmake', '--build', build_dir, '--parallel', job_count]).returncode != 0:
            return False
    return True

def cleanBuild():
    shutil.rmtree('build')
    shutil.rmtree('bin')

parser = ArgumentParser(description='This is a build tool.')
subparsers = parser.add_subparsers(dest='command')
compile = subparsers.add_parser('compile')
clean = subparsers.add_parser('clean')
assets = subparsers.add_parser('assets')
deps = subparsers.add_parser('deps')

args = parser.parse_args()

os.chdir(repoPath)

start_time = time.time()

if args.command == 'compile':
    build()
elif args.command == 'clean':
    cleanBuild()
elif args.command == 'assets':
    build_assets(False)
elif args.command == 'deps':
    fetch_dependencies()
else:
    fetch_dependencies()
    build_assets(False)
    if build():
        subprocess.run([os.path.abspath(os.path.join('bin', 'game'))], cwd=os.path.abspath('bin'))

print('Took {:.2f} seconds'.format(time.time() - start_time))
