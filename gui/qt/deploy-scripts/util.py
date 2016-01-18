import os
import subprocess
import errno

def simple_exec(cmd):
    print("   -> Executing command: %s" % " ".join(cmd))
    
    retcode = subprocess.call(cmd)
    
    if retcode != 0:
        print("   !! ERROR: Command returned exit code %i!" % retcode)
        print("   !!        Command: %s" % " ".join(cmd))
        return False
    
    return True

def silent_exec(cmd):
    print("   -> Executing command: %s" % " ".join(cmd))
    
    FNULL = open(os.devnull, 'w')
    retcode = subprocess.call(cmd, stdout=FNULL, stderr=subprocess.STDOUT)
    
    if retcode != 0:
        print("   !! ERROR: Command returned exit code %i!" % retcode)
        print("   !!        Command: %s" % " ".join(cmd))
        return False
    
    return True

def output_exec(cmd):
    print("   -> Executing command: %s" % " ".join(cmd))
    
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    out, err = p.communicate()
    
    if p.returncode != 0:
        print("   !! ERROR: Command returned exit code %i!" % retcode)
        print("   !!        Command: %s" % " ".join(cmd))
        return None
    
    return out

# tzot @ StackOverflow:
# http://stackoverflow.com/a/600612
def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def silentremove(filename):
    try:
        os.remove(filename)
    except OSError as e: # this would be "except OSError, e:" before Python 2.6
        if e.errno != errno.ENOENT: # errno.ENOENT = no such file or directory
            raise # re-raise exception if a different error occured