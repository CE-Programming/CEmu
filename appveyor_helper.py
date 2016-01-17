import os
import sys
import hashlib
import re
import subprocess
import time
import glob
import zipfile
import shutil
import errno
import requests

try:
    import zlib
    compression = zipfile.ZIP_DEFLATED
except:
    compression = zipfile.ZIP_STORED

modes = { zipfile.ZIP_DEFLATED: 'deflated',
          zipfile.ZIP_STORED:   'stored',
          }

try:
    # Python 3
    from urllib.request import urlopen, Request
    from urllib.error import HTTPError, URLError
except ImportError:
    # Python 2
    from urllib2 import urlopen, Request, HTTPError, URLError

INCLUDE_QT_LIBS =  "quickwidgets widgets gui qml core quick network"
INCLUDE_QT_LIBS += " qwindows qtquick2plugin qquicklayoutsplugin qdds qgif qicns qico qjpeg qsvg qtga qtiff qwbmp qwebp"
BINTRAY_SNAPSHOT_SERVER_PATH = "https://oss.jfrog.org/artifactory/oss-snapshot-local"
BINTRAY_RELEASE_SERVER_PATH = "https://oss.jfrog.org/artifactory/oss-release-local"
BINTRAY_MAVEN_GROUP_PATH = "/org/github/alberthdev/cemu/"
MAX_ATTEMPTS = 5

def truncate_url(url):
    if len(url) > 70:
        truncated_url = url[:34] + ".." + url[len(url) - 34:]
    else:
        truncated_url = url
    
    return truncated_url

def dlfile(url):
    dl_attempts = 0
    
    while dl_attempts <= MAX_ATTEMPTS:
        # If we aren't on our first download attempt, wait a bit.
        if dl_attempts > 0:
            print("         !! Download attempt failed, waiting 10s before retry...")
            print("            (attempt %i/%i)" % (dl_attempts + 1, MAX_ATTEMPTS))
            
            # Wait...
            time.sleep(10)
        
        # Open the url
        try:
            f = urlopen(url)
            print("         -> Downloading:")
            print("            %s" % truncate_url(url))

            # Open our local file for writing
            with open(os.path.basename(url), "wb") as local_file:
                local_file.write(f.read())
            
            # Everything good!
            break
            
        # Error handling...
        except HTTPError:
            _, e, _ = sys.exc_info()
            print("         !! HTTP Error: %i (%s)" % (e.code, url))
        except URLError:
            _, e, _ = sys.exc_info()
            print("         !! URL Error: %s (%s)", e.reason, url)
        
        # Increment attempts
        dl_attempts += 1
        
    if dl_attempts > MAX_ATTEMPTS:
        print("         !! ERROR: Download failed, exiting!")
        sys.exit(1)

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
    print("      -> Validating file with %s: %s" % (hash_name, filename))
    try:
        hash_fh = open(chksum_file)
        correct_hash = hash_fh.read().strip()
        hash_fh.close()
    except IOError:
        print("      !! ERROR: Could not open checksum file '%s' for opening!" % chksum_file)
        print("      !!        Exact error follows...")
        raise
    
    # Ensure hash is a valid checksum
    hash_match = re.match(hash_regex, correct_hash)

    if not hash_match:
        print("      !! ERROR: Invalid %s checksum!" % hash_name)
        print("      !!        Extracted %s (invalid): %s" % (hash_name, correct_hash))
        sys.exit(1)
    
    # One more thing - check to make sure the file exists!
    try:
        test_fh = open(filename, "rb")
        test_fh.close()
    except IOError:
        print("      !! ERROR: Can't check %s checksum - could not open file!" % hash_name)
        print("      !!        File: %s" % filename)
        print("      !! Traceback follows:")
        traceback.print_exc()
        return False
    
    # Alright, let's compute the checksum!
    cur_hash = hash_func(filename)
    
    # Check the checksum...
    if cur_hash != correct_hash:
        print("      !! ERROR: %s checksums do not match!" % hash_name)
        print("      !!        File: %s" % filename)
        print("      !!        Current %s: %s" % (hash_name, cur_hash))
        print("      !!        Correct %s: %s" % (hash_name, correct_hash))
        return False
    
    # Otherwise, everything is good!
    return True

def validate(filename):
    valid_md5 = validate_gen(filename, filename + ".md5", "MD5", r'^[0-9a-f]{32}$', generate_file_md5)
    
    if valid_md5:
        valid_sha1 = validate_gen(filename, filename + ".sha1", "SHA1", r'^[0-9a-f]{40}$', generate_file_sha1)
        
        return valid_sha1
    else:
        return False # alternatively, valid_md5

def dl_and_validate(url):
    validation_attempts = 0
    local_fn = os.path.basename(url)
    
    print("   -> Downloading + validating:")
    print("      %s" % truncate_url(url))
    
    # Download checksums...
    print("      -> Downloading checksums for file: %s" % (local_fn))
    dlfile(url + ".md5")
    dlfile(url + ".sha1")
    
    while validation_attempts < MAX_ATTEMPTS:
        # If we aren't on our first download attempt, wait a bit.
        if validation_attempts > 0:
            print("      !! Download + validation attempt failed, waiting 10s before retry...")
            print("         (attempt %i/%i)" % (validation_attempts + 1, MAX_ATTEMPTS))
            # Wait...
            time.sleep(10)
        
        print("      -> Downloading file: %s" % (local_fn))
        
        # Download file...
        dlfile(url)
        
        # ...and attempt to validate it!
        if validate(local_fn):
            break
        
        # Validation failed... looping back around.
        # Increment validation attempt counter
        validation_attempts += 1
    
    if validation_attempts > MAX_ATTEMPTS:
        print("      !! ERROR: Download and validation failed, exiting!")
        sys.exit(1)
    
    print("      -> Downloaded + validated successfully:")
    print("         %s" % truncate_url(url))

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

def extract(filename):
    print("   -> Extracting file: %s" % filename)
    if not silent_exec(["7z", "x", "-oC:\\", filename]):
        print("   !! ERROR: Failed to extract file: " % filename)
        print("   !!        See above output for details.")
        sys.exit(1)

def install_deps():
    print(" * Attempting to download dependencies...")
    dl_and_validate('https://oss.jfrog.org/artifactory/oss-snapshot-local/org/github/alberthdev/cemu/appveyor-qt/Qt56_Beta_Win32_DevDeploy.7z')
    dl_and_validate('https://oss.jfrog.org/artifactory/oss-snapshot-local/org/github/alberthdev/cemu/appveyor-qt/Qt56_Beta_Win64_DevDeploy.7z')
    
    print(" * Attempting to install dependencies...")
    extract('Qt56_Beta_Win32_DevDeploy.7z')
    extract('Qt56_Beta_Win64_DevDeploy.7z')
    
    print(" * Successfully installed build dependencies!")

def overwrite_copy(src, dest):
    src_bn = os.path.basename(src)
    dest_path = os.path.join(dest, src_bn)
    
    src_fh = open(src, "rb")
    dest_fh = open(dest_path, "wb")
    
    dest_fh.write(src_fh.read())
    
    src_fh.close()
    dest_fh.close()

# wc = wildcard
# extra_wc: dictionary of extra wildcard paths to copy in!
#   example: { "More DLLs" : "C:\MoreDLLs\*.dll"}
def collect_main_files(arch, vcredist_wc_path, ucrt_wc_path, build_path, dest, extra_wc = None):
    file_list = []
    
    print("   -> Searching VCRedist for DLL files to include (%s)..." % (arch))
    
    for file in glob.glob(vcredist_wc_path):
        print("   -> Copying %s (%s, VCRedist)..." % (os.path.basename(file), arch))
        overwrite_copy(file, dest)
    
    for file in glob.glob(ucrt_wc_path):
        print("   -> Copying %s (%s, UCRT)..." % (os.path.basename(file), arch))
        overwrite_copy(file, dest)
    
    if extra_wc:
        for copy_type in extra_wc:
            print("   -> Copying %s files (%s)..." % (copy_type, arch))
            copy_wc = extra_wc[copy_type]
            
            for file in glob.glob(copy_wc):
                print("      -> Copying %s (%s, %s)..." % (os.path.basename(file), arch, copy_type))
                overwrite_copy(file, dest)
        
    
    # Finally, add our binary!
    print("   -> Copying main executable (%s)..." % (arch))
    exec_path = os.path.join(build_path, "CEmu.exe")
    overwrite_copy(exec_path, dest)
    
    # No manifest needed - already embedded into exe.
    
def collect_qt_files(arch, deploy_tool, dest, exe_file):
    os.environ.pop("VCINSTALLDIR", None)
    print("   -> Collecting all Qt dependencies (%s)..." % (arch))
    if not simple_exec([deploy_tool, "--qmldir", "qml", "--dir", dest, exe_file]):
        print("   !! ERROR: Failed to collect Qt dependencies!")
        print("   !!        See above output for details.")
        sys.exit(1)

def build_file_list(arch, dest):
    file_list = []
    root_parts = len(dest.split(os.sep))
    
    print("   -> Finalizing file list for release (%s)..." % (arch))
    
    for root, dirnames, filenames in os.walk(dest):
        for filename in filenames:
            full_path = os.path.join(root, filename)
            # Delete root folder from path to form archive path!
            arc_path = os.sep.join(full_path.split(os.sep)[root_parts:])
            file_list.append([full_path, arc_path])
    
    return file_list

def make_zip(arch, filename, file_list):
    print(" * Building ZIP file %s (%s)..." % (filename, arch))
    
    if compression == zipfile.ZIP_DEFLATED:
        print("   (Compression is enabled!)")
    else:
        print("   (Compression is DISABLED!)")
    
    zf = zipfile.ZipFile(filename, mode='w')
    try:
        for file_entry in file_list:
            if len(file_entry) == 1:
                full_path = file_entry[0]
                arc_path = full_path
            elif len(file_entry) == 2:
                full_path = file_entry[0]
                arc_path = file_entry[1]
            else:
                print("   !! ERROR: Bug - invalid number of file elements in file_entry!")
                print("             file_entry is %s" % (str(file_entry)))
                sys.exit(1)
            
            print("   -> Adding %s -> %s..." % (full_path, arc_path))
            zf.write(full_path, compress_type=compression, arcname=arc_path)
    finally:
        print("   -> Closing ZIP file %s (%s)..." % (filename, arch))
        zf.close()
    
    print(" * Successfully built ZIP file %s (%s)!" % (filename, arch))

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

def upload_snapshot(filename, bintray_api_username, bintray_api_key, extra_path = None):
    full_path = BINTRAY_SNAPSHOT_SERVER_PATH + BINTRAY_MAVEN_GROUP_PATH
    base_fn = os.path.basename(filename)
    
    print(" * Preparing to deploy snapshot: %s" % base_fn)
    
    if extra_path:
        full_path += extra_path
    
    # Compute MD5 and SHA1
    print("   -> Computing checksums before uploading...")
    file_md5sum = generate_file_md5(filename)
    file_sha1sum = generate_file_sha1(filename)
    
    print("   -> MD5  = %s" % file_md5sum)
    print("   -> SHA1 = %s" % file_sha1sum)
    
    headers = {
                'X-Checksum-Md5'  : file_md5sum,
                'X-Checksum-Sha1' : file_sha1sum,
              }
    
    #files = {base_fn: open(filename, 'rb')}
    fh = open(filename, 'rb')
    file_data = fh.read()
    fh.close()
    
    print(" * Uploading/deploying snapshot: %s" % base_fn)
    r = requests.put(full_path + base_fn, headers = headers, data = file_data, \
                    auth = (bintray_api_username, bintray_api_key))
    
    print(" * Resulting status code: %i" % r.status_code)
    print(" * Resulting response:\n%s" % r.content)
    
    if r.status_code != 201:
        print(" ! ERROR: Upload/deployment of snapshot failed!")

def deploy_snapshots():
    print(" * Preparing to deploy...")
    
    # Check for our needed environment variables!
    bintray_api_username = os.environ.get("BINTRAY_API_USERNAME")
    bintray_api_key = os.environ.get("BINTRAY_API_KEY")
    
    if (bintray_api_username == None) or (bintray_api_key == None):
        print(" ! ERROR: Authentication environmental variables not found!")
        print(" !        BINTRAY_API_USERNAME defined? %s" % ("Yes" if bintray_api_username else "No"))
        print(" !        BINTRAY_API_KEY defined?      %s" % ("Yes" if bintray_api_key else "No"))
        sys.exit(1)
    
    # Make a directory for our deploy ZIPs
    mkdir_p("deploy")
    mkdir_p(os.path.join("deploy", "release32"))
    mkdir_p(os.path.join("deploy", "release64"))
    mkdir_p(os.path.join("deploy", "release32_debug"))
    mkdir_p(os.path.join("deploy", "release64_debug"))
    
    # git rev-parse --short HEAD
    git_rev = output_exec(["git", "rev-parse", "--short", "HEAD"])
    
    if git_rev == None:
        sys.exit(1)
    
    git_rev = git_rev.decode("utf-8").strip()
    
    # Snapshot filename - based on http://zeranoe1.rssing.com/chan-5973786/latest.php
    snap_base_fn = os.path.join("deploy", "cemu-%s-git%s-" % (time.strftime("%Y%m%d_%H%M%S"), git_rev))
    
    # Locate files that we need!
    print(" * Collecting all dependencies for deployment...")
    
    collect_main_files("x86", r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\*.dll",
                       r"C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\*.dll",
                       os.path.join("build_32", "release"),
                       os.path.join("deploy", "release32"))
    collect_main_files("x64", r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x64\Microsoft.VC140.CRT\*.dll",
                       r"C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64\*.dll",
                       os.path.join("build_64", "release"),
                       os.path.join("deploy", "release64"))
    
    # For debug builds, only copy api*.dll for UCRT redist, then copy
    # the specific ucrt debug DLL in the extra copy arg.
    collect_main_files("x86 Debug", r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\debug_nonredist\x86\Microsoft.VC140.DebugCRT\*.dll",
                       r"C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\api*.dll",
                       os.path.join("build_32", "release"),
                       os.path.join("deploy", "release32"),
                       extra_wc = {
                                    "UCRT Debug" : r"C:\Program Files (x86)\Windows Kits\10\bin\x86\ucrt\*.dll",
                                  }
                      )
    collect_main_files("x64 Debug", r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\debug_nonredist\x64\Microsoft.VC140.DebugCRT\*.dll",
                       r"C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x64\api*.dll",
                       os.path.join("build_64", "release"),
                       os.path.join("deploy", "release64"),
                       extra_wc = {
                                    "UCRT Debug" : r"C:\Program Files (x86)\Windows Kits\10\bin\x64\ucrt\*.dll",
                                  }
                      )
    
    collect_qt_files("x86", r"C:\Qt\Qt5.6.0\5.6\msvc2015\bin\windeployqt.exe", r"deploy\release32", r'build_32\release\CEmu.exe')
    collect_qt_files("x64", r"C:\Qt\Qt5.6.0x64\5.6\msvc2015_64\bin\windeployqt.exe", r"deploy\release64", r'build_64\release\CEmu.exe')
    
    collect_qt_files("x86 Debug", r"C:\Qt\Qt5.6.0\5.6\msvc2015\bin\windeployqt.exe", r"deploy\release32_debug", r'build_32\debug\CEmu.exe')
    collect_qt_files("x64 Debug", r"C:\Qt\Qt5.6.0x64\5.6\msvc2015_64\bin\windeployqt.exe", r"deploy\release64_debug", r'build_64\debug\CEmu.exe')
    
    file_list_32 = build_file_list("x86", r"deploy\release32")
    file_list_64 = build_file_list("x64", r"deploy\release64")
    
    file_list_32_debug = build_file_list("x86 Debug", r"deploy\release32_debug")
    file_list_64_debug = build_file_list("x64 Debug", r"deploy\release64_debug")
    
    # Build our ZIPs!
    cemu_win32_zip_fn = snap_base_fn + "win32-release-shared.zip"
    cemu_win64_zip_fn = snap_base_fn + "win64-release-shared.zip"
    
    cemu_win32_debug_zip_fn = snap_base_fn + "win32-debug-shared.zip"
    cemu_win64_debug_zip_fn = snap_base_fn + "win64-debug-shared.zip"
    
    make_zip("x86", cemu_win32_zip_fn, file_list_32)
    make_zip("x64", cemu_win64_zip_fn, file_list_64)
    
    make_zip("x86 Debug", cemu_win32_debug_zip_fn, file_list_32_debug)
    make_zip("x64 Debug", cemu_win64_debug_zip_fn, file_list_64_debug)
    
    # Upload everything!
    upload_snapshot(cemu_win32_zip_fn, bintray_api_username, bintray_api_key)
    upload_snapshot(cemu_win64_zip_fn, bintray_api_username, bintray_api_key)
    
    upload_snapshot(cemu_win32_debug_zip_fn, bintray_api_username, bintray_api_key)
    upload_snapshot(cemu_win64_debug_zip_fn, bintray_api_username, bintray_api_key)
    
    print(" * Snapshot deployment complete!")
    
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
    elif sys.argv[1] == "deploy":
        deploy_snapshots()
    else:
        usage("ERROR: Invalid command!")