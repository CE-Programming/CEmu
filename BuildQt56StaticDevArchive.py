import os
import sys
import fnmatch
import errno
import subprocess
import shutil
import re
import glob

SEVENZIP="C:\\Program Files\\7-Zip\\7z.exe"
QTBASEDIR="C:\\"
QT32 = "Qt\\Qt5.6.0-static"
SSL32 = "OpenSSL-win32"
SSL64 = "OpenSSL-win64"
QT64 = "Qt\\Qt5.6.0x64-static"
LIB_DLL_DIR = ""
ARC_PREFIX = "Qt56_Beta_Static_"
ARC_SUFFIX_DEV = "_DevDeploy"

# What libraries need to be included?
# Note that this usually involves more than the libraries specified
# in your project file - you basically need to specify all of the
# libraries that you have to copy over in order for your program to
# run, minus "Qt5" and "dll"!
QT_LIB_INCLUDE = "core gui quick widgets quickwidgets network qml"

# Binary excludes
# (Don't change unless you know what you're doing!)
BIN_EXCLUDES = [ "assistant.exe", "designer.exe", "idc.exe", "idc.pdb", "linguist.exe", "moc.pdb", "pixeltool.exe", "qdbus*", "rcc.pdb", "testcon*", "windeployqt*"]

def silentremove(filename):
    try:
        os.remove(filename)
    except OSError as e: # this would be "except OSError, e:" before Python 2.6
        if e.errno != errno.ENOENT: # errno.ENOENT = no such file or directory
            raise # re-raise exception if a different error occured

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

def collect_qt_static_files(proj_file):
    proj_fh = open(proj_file, "r")
    
    all_matches = []
    all_matches_arr = []
    all_libs = []
    
    for line in proj_fh:
        re_searches = re.search(r'\<AdditionalDependencies\>(.*)\<\/AdditionalDependencies\>', line)
        if re_searches:
            matches = re_searches.groups()
            all_matches_arr.append(list(matches))
    
    # Flatten array
    for matches in all_matches_arr:
        all_matches += matches
    
    # Split into filenames
    for libs in all_matches:
        all_libs += libs.split(os.pathsep)
    
    # Filter out variables, and get basenames!
    # Also, filter out non-Qt variables
    all_libs = [os.path.basename(p) for p in all_libs if not p.startswith("%")]
    
    # Remove .lib extensions AND filter only Qt libs
    # Also, make everything lowercase!
    all_libs = [".".join(p.split(".")[:-1]).lower() for p in all_libs if p.lower().startswith("q")]
    
    # Interesting method - seperate and filter "d" libraries,
    # make sure we're really removing DEBUG libraries. If a library
    # happens to end with "d", then we'll preserve it.
    
    # Seperate libs that end with 'd'
    all_d_libs = [p for p in all_libs if p.lower().endswith("d")]
    all_libs = [p for p in all_libs if not p.lower().endswith("d")]
    
    # Remove d from libs
    all_d_libs = [p[:-1] for p in all_d_libs]
    
    # If any of these match an element inside all_libs, remove it!
    all_d_libs = [p for p in all_d_libs if not p in all_libs]
    
    # Adding back "d" for libraries that still exist!
    all_d_libs = [p+"d" for p in all_d_libs]
    
    # Merge libraries
    all_libs = all_libs + all_d_libs
    
    # Remove duplicates
    all_libs = list(set(all_libs))
    
    return all_libs

# When splitting a 7z file, it will always come with
# a *.001 extension, even if it never makes the split limit.
# As such, we want to detect that and rename it to just .7z.
# This function does exactly that - given ARC.7z, rename
# ARC.7z.001 to ARC.7z if that's the only file.
def move_single_7z_file(filename):
    basename_fn = os.path.basename(filename)
    file_list = glob.glob(filename + "*")
    if len(file_list) == 0:
        print("   -> ERROR: File (prefix) %s doesn't exist!" % filename)
        sys.exit(1)
    
    if len(file_list) == 1:
        print("   -> Renaming: %s -> %s (only one part detected)" % (os.path.basename(file_list[0]), basename_fn))
        os.rename(file_list[0], filename)
    else:
        print("   -> Not renaming: %s (found %i parts!)" % (os.path.basename(file_list[0]), len(file_list)))

def silent_remove_wildcard(file_wc):
    for f in glob.glob(file_wc):
        silentremove(f)

print("====================================================")
print("= Building Qt v5.6 development archive (static)... =")
print("====================================================")

cdir = os.getcwd()

print(" * Stage 0: Removing Old Archives")
silent_remove_wildcard(os.path.join(cdir, ARC_PREFIX + "Win32" + ARC_SUFFIX_DEV + "*.7z*"))
silent_remove_wildcard(os.path.join(cdir, ARC_PREFIX + "Win64" + ARC_SUFFIX_DEV + "*.7z*"))

print(" * Stage 1: Analyzing CEmu")
mkdir_p("build_32")

os.chdir("build_32")
if not simple_exec([r'C:\Qt\Qt5.6.0-static\bin\qmake', '-spec', 'win32-msvc2015', '-tp', 'vc', r'"..\CEmu.pro"']):
    print(" ! ERROR: Creating project files for x86 failed!")
    sys.exit(1)
os.chdir("..")

print(" * Stage 2: Building Exclusion List")

print("   -> Stage 2a: Collecting Qt Dependencies")
cemu32_proj = os.path.join("build_32", "CEmu.vcxproj")

found_libs = collect_qt_static_files(cemu32_proj)

print("   -> Stage 2b: Creating Exclusion List")

# First, find all of the DLL files
lib_matches = []
pdb_matches = []

# Is it bad to just base everything off of 32-bit? Assuming libraries
# have the same version, maybe not...

for root, dirnames, filenames in os.walk(os.path.join(QTBASEDIR, QT32, LIB_DLL_DIR)):
    for filename in fnmatch.filter(filenames, '*.lib'):
        lib_matches.append(os.path.join(root, filename))

for root, dirnames, filenames in os.walk(os.path.join(QTBASEDIR, QT32, LIB_DLL_DIR)):
    for filename in fnmatch.filter(filenames, '*.pdb'):
        pdb_matches.append(os.path.join(root, filename))

# Get the actual DLL filenames
# Result: Qt5Gui.dll, Qt5Guid.dll, etc.
# (Note that the debug DLLs are still in there)
lib_matches = [p.split(os.sep)[-1] for p in lib_matches]
pdb_matches = [p.split(os.sep)[-1] for p in pdb_matches]

# Remove debug LIBs from the list
# LIBs that remain, strip off the file extension.
# Result: Qt5Gui, Qt5Help, etc.
lib_matches = [".".join(p.split(".")[:-1]) for p in lib_matches if not p.lower().endswith("d.lib")]

# We don't filter PDB matches because PDBs are always debug = d.xxx
# That said, let's snip off the "d.pdb" part!
# Do like the above - strip off the file extensions.
# Then strip off the last character = non-debug!
pdb_matches = [".".join(p.split(".")[:-1])[:-1] for p in pdb_matches]

# Filter libraries that are "Info.plist"
lib_matches = [p for p in lib_matches if p != "Info.plist"]

qt_lib_include_arr = [ ("qt5" + lib.lower()) for lib in QT_LIB_INCLUDE.split(" ") ] + found_libs

# Remove the DLLs that are included in the config above, and leave
# the ones to exclude
lib_matches = [p for p in lib_matches if not p.lower() in qt_lib_include_arr]
pdb_matches = [p for p in pdb_matches if not p.lower() in qt_lib_include_arr]

# Filter pdb files to exclude DLL *and* LIB matches
pdb_matches = [p for p in pdb_matches if not p in lib_matches]

# Combine all matches together!
prefix_matches = lib_matches + pdb_matches

# Ensure that qtmain is NOT excluded!
# (It shouldn't, but just in case...)
if "qtmain" in prefix_matches:
    prefix_matches.remove("qtmain")

# Ensure EGL/libGLESv2 are NOT excludes... but only if we have
# QtQuick/OpenGL involved.
qt_lib_include_arr_quick = [p for p in qt_lib_include_arr if "quick" in p.lower()]
if len(qt_lib_include_arr_quick) > 0:
    if "libEGL" in prefix_matches:
        prefix_matches.remove("libEGL")
    if "libGLESv2" in prefix_matches:
        prefix_matches.remove("libGLESv2")
    if "opengl32sw" in prefix_matches:
        prefix_matches.remove("opengl32sw")
    
    # Do a list comp to remove d3dcompiler_XX
    prefix_matches = [p for p in prefix_matches if not p.lower().startswith("d3dcompiler_")]

# Create exclusion commands!
dll_excludes_nondebug = ["-xr!" + p + ".dll" for p in prefix_matches]
dll_excludes_debug = ["-xr!" + p + "d.dll" for p in prefix_matches]
lib_excludes_nondebug = ["-xr!" + p + ".lib" for p in prefix_matches]
lib_excludes_debug = ["-xr!" + p + "d.lib" for p in prefix_matches]
prl_excludes_nondebug = ["-xr!" + p + ".prl" for p in prefix_matches]
prl_excludes_debug = ["-xr!" + p + "d.prl" for p in prefix_matches]
pdb_excludes = ["-xr!" + p + "d.pdb" for p in prefix_matches]

all_excludes = dll_excludes_nondebug + dll_excludes_debug + \
                lib_excludes_nondebug + lib_excludes_debug + \
                prl_excludes_nondebug + prl_excludes_debug + pdb_excludes

if (not ("webengine" in qt_lib_include_arr)) and (not ("webenginecore" in qt_lib_include_arr)):
    all_excludes.append("-xr!qtwebengine*")

all_excludes = all_excludes + ["-xr!" + e for e in BIN_EXCLUDES]

shutil.rmtree("build_32")

# To preserve path in 7-Zip, you MUST change directory to where the
# folder is. If you don't, the first part of the path will be lost.
os.chdir(QTBASEDIR)

# IMPORTANT NOTE: All paths must be in quotes! Otherwise, 7-Zip will
# NOT parse the paths correctly, and weird things will happen!

print(" * Stage 3: Building Development Archive (x86)")
subprocess.call([SEVENZIP, "a", os.path.join(cdir, ARC_PREFIX + "Win32" + ARC_SUFFIX_DEV + ".7z"), SSL32, QT32,
                "-t7z", "-m0=lzma2", "-mx9", "-v100m",
                    # Exclude unnecessary files
                    
                    # Installation files
                    "-xr!" + os.path.join(QT32, "network.xml"),
                    "-xr!" + os.path.join(QT32, "Maintenance*"),
                    "-xr!" + os.path.join(QT32, "InstallationLog.txt"),
                    "-xr!" + os.path.join(QT32, "components.xml"),
                    
                    # Tools, examples, documentation, and VCRedist
                    "-xr!" + os.path.join(QT32, "Tools"),
                    "-xr!" + os.path.join(QT32, "Examples"),
                    "-xr!" + os.path.join(QT32, "Docs"),
                    "-xr!" + os.path.join(QT32, "vcredist"),
                    "-xr!" + os.path.join(QT32, LIB_DLL_DIR, "doc"),
                    
                    #"-xr!*d.dll",
                    #"-xr!*d.lib",
                    #"-xr!*.pdb",
                ] + all_excludes)

# -xr!*d.dll -xr!*d.lib -xr!*.pdb

move_single_7z_file(os.path.join(cdir, ARC_PREFIX + "Win32" + ARC_SUFFIX_DEV + ".7z"))

print(" * Stage 4: Building Development Archive (x64)")
subprocess.call([SEVENZIP, "a", os.path.join(cdir, ARC_PREFIX + "Win64" + ARC_SUFFIX_DEV + ".7z"), SSL64, QT64,
                "-t7z", "-m0=lzma2", "-mx9", "-v100m",
                    # Exclude unnecessary files
                    
                    # Installation files
                    "-xr!" + os.path.join(QT64, "network.xml"),
                    "-xr!" + os.path.join(QT64, "Maintenance*"),
                    "-xr!" + os.path.join(QT64, "InstallationLog.txt"),
                    "-xr!" + os.path.join(QT64, "components.xml"),
                    
                    # Tools, examples, documentation, and VCRedist
                    "-xr!" + os.path.join(QT64, "Tools"),
                    "-xr!" + os.path.join(QT64, "Examples"),
                    "-xr!" + os.path.join(QT64, "Docs"),
                    "-xr!" + os.path.join(QT64, "vcredist"),
                    "-xr!" + os.path.join(QT64, LIB_DLL_DIR, "doc"),
                    
                    #"-xr!*d.dll",
                    #"-xr!*d.lib",
                    #"-xr!*.pdb",
                ] + all_excludes)

move_single_7z_file(os.path.join(cdir, ARC_PREFIX + "Win64" + ARC_SUFFIX_DEV + ".7z"))

print(" * All done!")