import os
import sys
import fnmatch
import subprocess
import shutil
import re
import glob

from difflib import SequenceMatcher

from util import *

DEBUG_ENABLED = False
SEVENZIP="C:\\Program Files\\7-Zip\\7z.exe"
QTBASEDIR="C:\\"
QT32 = "Qt\\Qt5.11.1-static"
SSL32 = "LibreSSL-win32"
SSL64 = "LibreSSL-win64"
QT64 = "Qt\\Qt5.11.1x64-static"
LIB_DLL_DIR = ""
ARC_PREFIX = "Qt5.11.1_Rel_Static_"
ARC_SUFFIX_DEV = "_DevDeploy"

# What libraries need to be included?
# You don't have to specify every library necessary to make things run.
# You can simply specify the libraries in your .pro file, and this script
# will do the rest. (It detects subdependencies on its own!)
QT_LIB_INCLUDE = "core gui quick widgets quickwidgets network qml"

# Binary excludes
# (Don't change unless you know what you're doing!)
BIN_EXCLUDES = [ "assistant.exe", "designer.exe", "idc.exe", "idc.pdb", "linguist.exe", "moc.pdb",
                 "pixeltool.exe", "qdbus*", "rcc.pdb", "testcon*", "windeployqt*"]

# Don't change this - we need this to check
WINDOW_CMDLINE_LIMIT = 32767

def subprocess_call(*args, **kwargs):
    string_cmd = " ".join(args[0])
    if DEBUG_ENABLED:
        print("Running command: %s (%s chars)" % (string_cmd, len(" ".join(args[0]))))
    if len(string_cmd) > WINDOW_CMDLINE_LIMIT:
        raise RuntimeError("command given is larger than %d chars, which is the Windows limit (given: %d chars)" %
                           (WINDOW_CMDLINE_LIMIT, len(string_cmd)))
    return subprocess.call(*args, **kwargs)

# String stuff

# Based on commonprefix from os.path
def commonsuffix(m):
    "Given a list of pathnames, returns the longest common tailing component"
    rev_m = [s[::-1] for s in m]
    return os.path.commonprefix(rev_m)[::-1]

# String similarity
def similar_str(a, b):
    return SequenceMatcher(None, a, b).ratio()

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
print("= Building Qt v5.11 development archive (static)   =")
print("====================================================")

# Modify PATH if needed
entire_path = os.environ["PATH"]
entire_path = entire_path.split(os.pathsep)
entire_path = [p for p in entire_path if "PyQt5" not in p]
entire_path = os.pathsep.join(entire_path)
os.environ["PATH"] = entire_path

cdir = os.getcwd()

print(" * Stage 0: Removing Old Archives")
silent_remove_wildcard(os.path.join(cdir, ARC_PREFIX + "Win32" + ARC_SUFFIX_DEV + "*.7z*"))
silent_remove_wildcard(os.path.join(cdir, ARC_PREFIX + "Win64" + ARC_SUFFIX_DEV + "*.7z*"))

print(" * Stage 1: Analyzing CEmu")
os.chdir(os.path.dirname(os.path.realpath(__file__)))
mkdir_p("build_32")

os.chdir("build_32")
print(os.getcwd())

REQUIRED_ENV_VARS = [
    ["ZLIB_LIB", "LIBPNG_APNG_LIB", "LIBPNG_APNG_INCLUDE"],
    ["LIBPNG_APNG_FROM_VCPKG"]
]

if not any(all([True if p in os.environ else False for p in envset]) for envset in REQUIRED_ENV_VARS):
    print(" ! ERROR: These environment variables for 32-bit builds must be defined for detection to work: %s" %
          " OR ".join("(%s)" % ", ".join(envset) for envset in REQUIRED_ENV_VARS))
    sys.exit(1)

if "LIBPNG_APNG_LIB" in os.environ:
    if not simple_exec([r'C:\Qt\Qt5.11.1-static\bin\qmake', '-spec', 'win32-msvc', '-tp', 'vc',
                       'LIBS+=' + os.environ.get('ZLIB_LIB'), 'LIBS+=' + os.environ.get('LIBPNG_APNG_LIB'),
                       'INCLUDEPATH+=' + os.environ.get('LIBPNG_APNG_INCLUDE'), r'..\..\CEmu.pro']):
        print(" ! ERROR: Creating project files for x86 failed (using LIBPNG_APNG_LIB env)!")
        sys.exit(1)
else:
    if not simple_exec([r'C:\Qt\Qt5.11.1-static\bin\qmake', '-spec', 'win32-msvc', '-tp', 'vc',
                        'LIBPNG_APNG_FROM_VCPKG=' + os.environ.get('LIBPNG_APNG_FROM_VCPKG'),
                         r'..\..\CEmu.pro']):
        print(" ! ERROR: Creating project files for x86 failed (using LIBPNG_APNG_FROM_VCPKG env)!")
        sys.exit(1)
os.chdir("..")

print(" * Stage 2: Building Exclusion List")

print("   -> Stage 2a: Collecting Qt Dependencies")
cemu32_proj = os.path.join("build_32", "CEmu.vcxproj")

found_libs = collect_qt_static_files(cemu32_proj)

print("   -- Found %d Qt libraries to include." % len(found_libs))

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

# Filter prefixes down and try to generate shorter wildcard excludes instead
prefix_matches_i = 0

possible_wcs = set()
elements_wcs = set()

while prefix_matches_i < len(prefix_matches):
    first_prefix_match = prefix_matches[prefix_matches_i]
    
    prefix_matches_j = prefix_matches_i + 1
    
    while prefix_matches_j < len(prefix_matches):
        second_prefix_match = prefix_matches[prefix_matches_j]
        similarity = similar_str(first_prefix_match, second_prefix_match)
        if DEBUG_ENABLED: print("%s vs %s = %.2f" % (first_prefix_match, second_prefix_match, similarity))
        if similarity < 0.5:
            prefix_matches_j -= 1
            break
        
        prefix_matches_j += 1
    
    possible_common_prefix_matches = prefix_matches[prefix_matches_i:prefix_matches_j+1]
    
    if len(possible_common_prefix_matches) > 1:
        common_prefix = os.path.commonprefix(possible_common_prefix_matches)
        common_suffix = commonsuffix(possible_common_prefix_matches)
        
        if DEBUG_ENABLED:
            print("Checking: %s" % ", ".join(possible_common_prefix_matches))
            print("Common prefix: %s" % common_prefix)
            print("Common suffix: %s" % common_suffix)
        
        possible_wcs.add((common_prefix, common_suffix))
        elements_wcs.update(possible_common_prefix_matches)
        
        prefix_matches_i = prefix_matches_j
    
    prefix_matches_i += 1

non_wcs = set(prefix_matches).difference(elements_wcs)
final_wcs = set()

if DEBUG_ENABLED:
    print("non_wcs = %s" % non_wcs)
    print("qt_lib_include_arr = %s" % qt_lib_include_arr)

lc_prefix_matches = [pm.lower() for pm in prefix_matches]

for possible_wc_tuple in possible_wcs:
    possible_wc = "{0}*{1}".format(*possible_wc_tuple)
    if DEBUG_ENABLED: print(possible_wc)
    too_broad = False
    for lib_match in qt_lib_include_arr:
        if fnmatch.fnmatch(lib_match.lower(), possible_wc.lower()) and lib_match not in lc_prefix_matches:
            if DEBUG_ENABLED: print("too broad, removing (%s matched by wc %s)" % (lib_match, possible_wc))
            too_broad = True
            break
    if not too_broad:
        final_wcs.add(possible_wc)

if DEBUG_ENABLED: print("final_wcs = %s" % final_wcs)

if DEBUG_ENABLED: print("** pre prefix_matches = %s" % prefix_matches)
pre_prefix_matches_len = len(prefix_matches)

prefix_matches = [prefix_match for prefix_match in prefix_matches
                  if not any(fnmatch.fnmatch(prefix_match, final_wc) for final_wc in final_wcs)]
prefix_matches += final_wcs

if DEBUG_ENABLED: print("** post prefix_matches = %s" % prefix_matches)

print("   -- Using %d specific wildcards, transformed %d excludes to %d excludes." % (len(final_wcs), pre_prefix_matches_len, len(prefix_matches)))

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

print("   -- Excluding %d files/wildcards from the archive." % len(all_excludes))

shutil.rmtree("build_32")

# To preserve path in 7-Zip, you MUST change directory to where the
# folder is. If you don't, the first part of the path will be lost.
os.chdir(QTBASEDIR)

# IMPORTANT NOTE: All paths must be in quotes! Otherwise, 7-Zip will
# NOT parse the paths correctly, and weird things will happen!

print(" * Stage 3: Building Development Archive (x86)")
subprocess_call([SEVENZIP, "a", os.path.join(cdir, ARC_PREFIX + "Win32" + ARC_SUFFIX_DEV + ".7z"), SSL32, QT32,
                "-t7z", "-m0=lzma2", "-mx9", "-v100000000",
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
subprocess_call([SEVENZIP, "a", os.path.join(cdir, ARC_PREFIX + "Win64" + ARC_SUFFIX_DEV + ".7z"), SSL64, QT64,
                "-t7z", "-m0=lzma2", "-mx9", "-v100000000",
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