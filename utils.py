import functools, subprocess, os, errno

@functools.lru_cache(maxsize=None)
def getRepoPath():
    return subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).decode('utf-8').strip()

def ensureDir(dir):
    try:
        os.makedirs(dir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

