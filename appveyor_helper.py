import os
import sys
import hashlib
import re
import subprocess

try:
    # Python 3
    from urllib.request import urlopen, Request
    from urllib.error import HTTPError, URLError
except ImportError:
    # Python 2
    from urllib2 import urlopen, Request, HTTPError, URLError

MAX_ATTEMPTS = 5

def dlfile(url):
    dl_attempts = 0
    
    while dl_attempts <= MAX_ATTEMPTS:
        # If we aren't on our first download attempt, wait a bit.
        if dl_attempts > 0:
            print("   !! Download attempt failed, waiting 10s before retry...")
            
            # Wait...
            time.sleep(10)
        
        # Open the url
        try:
            f = urlopen(url)
            print("   -> Downloading (attempt %i/%i):" % (dl_attempts + 1, MAX_ATTEMPTS))
            print("      %s" % url)

            # Open our local file for writing
            with open(os.path.basename(url), "wb") as local_file:
                local_file.write(f.read())
            
            # Everything good!
            break
            
        # Error handling...
        except HTTPError:
            _, e, _ = sys.exc_info()
            print("   !! HTTP Error: %i (%s)" % (e.code, url))
        except URLError:
            _, e, _ = sys.exc_info()
            print("   !! URL Error: %s (%s)", e.reason, url)
        
        # Increment attempts
        dl_attempts += 1
        
    if dl_attempts > MAX_ATTEMPTS:
        print("   !! ERROR: Download failed, exiting!")
        sys.exit(1)
    
    print("   -> Downloaded successfully: %s" % url)

def extractfile(filename):
    print("   -> Extracting file: %s" % filename)
    
    FNULL = open(os.devnull, 'w')
    retcode = subprocess.call(["7z", "x", "-oC:\\", filename], stdout=FNULL, stderr=subprocess.STDOUT)
    if retcode != 0:
        print("   !! ERROR: Extraction failed, see above messages for details. Exiting!")
    
    print("   -> Extracted successfully: %s" % filename)

def generate_file_md5(filename, blocksize=2**20):
    m = hashlib.md5()
    with open( filename , "rb" ) as f:
        while True:
            buf = f.read(blocksize)
            if not buf:
                break
            m.update( buf )
    return m.hexdigest()

def generate_file_sha1(filename, blocksize=2**20):
    m = hashlib.sha1()
    with open( filename , "rb" ) as f:
        while True:
            buf = f.read(blocksize)
            if not buf:
                break
            m.update( buf )
    return m.hexdigest()

def output_md5(filename):
    md5_result = "%s  %s" % (filename, generate_file_md5(filename))
    print(md5_result)
    return md5_result

def output_sha1(filename):
    sha1_result = "%s  %s" % (filename, generate_file_sha1(filename))
    print(sha1_result)
    return sha1_result

def make_checksum():
    md5_fh = open("Qt56_Beta_MD5SUMS.txt", "w")
    md5_fh.write(output_md5('Qt56_Beta_Win32_DevDeploy.7z') + "\n")
    md5_fh.write(output_md5('Qt56_Beta_Win64_DevDeploy.7z'))
    md5_fh.close()
    
    sha1_fh = open("Qt56_Beta_SHA1SUMS.txt", "w")
    sha1_fh.write(output_sha1('Qt56_Beta_Win32_DevDeploy.7z') + "\n")
    sha1_fh.write(output_sha1('Qt56_Beta_Win64_DevDeploy.7z'))
    sha1_fh.close()
    
    print("Saved to Qt56_Beta_MD5SUMS.txt and Qt56_Beta_SHA1SUMS.txt.")

# True if valid, False otherwise
# Generalized validation function
#   filename    - file to check
#   chksum_file - checksum file to verify against
#   hash_name   - name of hash function used
#   hash_regex  - regex to validate the hash format
#   hash_func   - function to create hash from file
def validate_gen(filename, chksum_file, hash_name, hash_regex, hash_func):
    print("   -> Validating file: %s" % filename)
    try:
        hash_fh = open(chksum_file)
    except IOError:
        print("   !! ERROR: Could not open checksum file '%s' for opening!" % chksum_file)
        print("   !!        Exact error follows...")
        raise
    
    found_file_hash = False
    
    for hash_line in hash_fh:
        min_hash_line = hash_line.strip()
        if min_hash_line != "":
            if "  " in min_hash_line:
                hash_arr = hash_line.split("  ")
            else:
                print("   !! ERROR: Invalid %s checksum line! (No double-space detected!)" % hash_name)
                print("   !!        Line: %s" % (min_hash_line))
                sys.exit(1)
            
            if len(hash_arr) != 2:
                print("   !! ERROR: Invalid %s checksum line! (Too many elements!)" % hash_name)
                print("   !!        Line: %s" % (min_hash_line))
                sys.exit(1)
            
            # Extract info
            hash_chksum = hash_arr[1].strip()
            hash_fn = hash_arr[0].strip()
            
            # Ensure hash is a valid checksum
            hash_match = re.match(hash_regex, hash_chksum)
            
            if not hash_match:
                print("   !! ERROR: Invalid %s checksum!" % hash_name)
                print("   !!        Line: %s" % (min_hash_line))
                print("   !!        Extracted %s (invalid): %s" % (hash_name, hash_chksum))
                sys.exit(1)
            
            # Is this the file we're looking for?
            if filename == hash_fn.strip():
                found_file_hash = True
                
                # One more thing - check to make sure the file exists!
                try:
                    test_fh = open(filename, "rb")
                    test_fh.close()
                except IOError:
                    print("   !! ERROR: Can't check %s checksum - could not open file!" % hash_name)
                    print("   !!        File: %s" % filename)
                    print("   !! Traceback follows:")
                    traceback.print_exc()
                    return False
                
                # Alright, let's compute the checksum!
                cur_hash = hash_func(filename)
                
                # Check the checksum...
                if cur_hash != hash_chksum:
                    print("   !! ERROR: %s checksums do not match!" % hash_name)
                    print("   !!        File: %s" % filename)
                    print("   !!        Current %s: %s" % (hash_name, cur_hash))
                    print("   !!        Correct %s: %s" % (hash_name, hash_chksum))
                    return False
                
                # We're good! Break out of loop!
                break
    
    if not found_file_hash:
        print("   !! ERROR: %s checksum for file could not be found!" % hash_name)
        print("   !!        File: %s" % filename)
        sys.exit(1)
    
    # Otherwise, everything is good!
    return True

def validate(filename):
    valid_md5 = validate_gen(filename, "Qt56_Beta_MD5SUMS.txt", "MD5", r'^[0-9a-f]{32}$', generate_file_md5)
    
    if valid_md5:
        valid_sha1 = validate_gen(filename, "Qt56_Beta_SHA1SUMS.txt", "SHA1", r'^[0-9a-f]{40}$', generate_file_sha1)
        
        return valid_sha1
    else:
        return False # alternatively, valid_md5

def dl_and_validate(url):
    validation_attempts = 0
    local_fn = os.path.basename(url)
    
    while validation_attempts < MAX_ATTEMPTS:
        # If we aren't on our first download attempt, wait a bit.
        if validation_attempts > 0:
            print("   !! Download + validation attempt failed, waiting 10s before retry...")
            
            # Wait...
            time.sleep(10)
        
        print("   -> Downloading + validating (attempt %i/%i): %s" % (validation_attempts + 1, MAX_ATTEMPTS, url))
        
        # Download file...
        dlfile(url)
        
        # ...and attempt to validate it!
        if validate(local_fn):
            break
        
        # Validation failed... looping back around.
        # Increment validation attempt counter
        validation_attempts += 1
    
    if validation_attempts > MAX_ATTEMPTS:
        print("   !! ERROR: Download and validation failed, exiting!")
        sys.exit(1)

def silent_exec(cmd):
    print("   -> Executing command: %s" % " ".join(cmd))
    
    FNULL = open(os.devnull, 'w')
    retcode = subprocess.call(cmd, stdout=FNULL, stderr=subprocess.STDOUT)
    
    if retcode != 0:
        print("   !! ERROR: Command return %i exit code!" % retcode)
        print("   !!        Command: %s" % " ".join(cmd))
        return False
    
    return True

def extract(filename):
    print("   -> Extracting file: %s" % filename)
    if not silent_exec(["7z", "x", "-oC:\\", filename]):
        print("   !! ERROR: Failed to extract file: " % filename)
        print("   !!        See above output for details.")
        sys.exit(1)

def install_deps():
    print(" * Attempting to download dependency checksums...")
    dlfile('https://oss.jfrog.org/artifactory/oss-snapshot-local/org/github/alberthdev/cemu/appveyor-qt/Qt56_Beta_MD5SUMS.txt')
    dlfile('https://oss.jfrog.org/artifactory/oss-snapshot-local/org/github/alberthdev/cemu/appveyor-qt/Qt56_Beta_SHA1SUMS.txt')
    
    print(" * Attempting to download dependencies...")
    dl_and_validate('https://oss.jfrog.org/artifactory/oss-snapshot-local/org/github/alberthdev/cemu/appveyor-qt/Qt56_Beta_Win32_DevDeploy.7z')
    dl_and_validate('https://oss.jfrog.org/artifactory/oss-snapshot-local/org/github/alberthdev/cemu/appveyor-qt/Qt56_Beta_Win64_DevDeploy.7z')
    
    print(" * Attempting to install dependencies...")
    extract('Qt56_Beta_Win32_DevDeploy.7z')
    extract('Qt56_Beta_Win64_DevDeploy.7z')
    
    print(" * Successfully installed build dependencies!")

def usage(msg = None):
    if msg:
        print(msg)
    
    print("Usage: %s [make_checksum|install|deploy]" % sys.argv[0])
    sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        usage()
        sys.exit(1)
    
    if sys.argv[1] == "make_checksum":
        make_checksum()
    elif sys.argv[1] == "install":
        install_deps()
    else:
        usage("ERROR: Invalid command!")