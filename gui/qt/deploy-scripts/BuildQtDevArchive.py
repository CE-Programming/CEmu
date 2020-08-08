import os
import sys
import fnmatch
import logging
import subprocess
import shutil
import re
import glob

import argparse
import configargparse

from difflib import SequenceMatcher

from util import *

DEFAULT_SEVENZIP="C:\\Program Files\\7-Zip\\7z.exe"
DEFAULT_ROOT_DIR="C:\\"
DEFAULT_QT_BASE_DIR="Qt"
DEFAULT_QT_DIR = "Qt{qt_version}"
DEFAULT_QT_LIB_DLL_SUBDIR = ""

# What libraries need to be included?
# You don't have to specify every library necessary to make things run.
# You can simply specify the libraries in your .pro file, and this script
# will do the rest. (It detects subdependencies on its own!)
DEFAULT_QT_LIB_INCLUDE = "core gui quick widgets quickwidgets network qml".split(" ")

# Binary excludes
# (Don't change unless you know what you're doing!)
DEFAULT_ARCHIVE_EXCLUDE = [ "assistant.exe", "designer.exe", "idc.exe", "idc.pdb", "linguist.exe", "moc.pdb",
                            "pixeltool.exe", "qdbus*", "rcc.pdb", "testcon*", "windeployqt*"]

DEFAULT_OUTPUT_ARCHIVE = "Qt{qt_version}_Rel_Static_Win32_DevDeploy.7z"

# Parses --opt KEY1=VALUE1 --opt KEY2=VALUE2 -> {KEY1: VALUE1, KEY2: VALUE2}
# https://stackoverflow.com/a/56521375/1094484
class kvdictAppendAction(argparse.Action):
    """
    argparse action to split an argument into KEY=VALUE form
    on the first = and append to a dictionary.
    """
    def __call__(self, parser, args, values, option_string=None):
        assert(len(values) == 1)
        try:
            (k, v) = values[0].split("=", 2)
        except ValueError as ex:
            raise argparse.ArgumentError(self, f"could not parse argument \"{values[0]}\" as k=v format")
        d = getattr(args, self.dest) or {}
        d[k] = v
        setattr(args, self.dest, d)

def parse_kv_to_dict(kv_list):
    d = {}
    for kv in kv_list:
        try:
            (k, v) = kv[0].split("=", 2)
        except ValueError as ex:
            raise RuntimeError(f"could not parse argument \"{kv[0]}\" as k=v format")
        d[k] = v
    return d

def parse_args():
    parser = configargparse.ArgParser(description="Tool to generate a Qt library archive with only the dependencies required "
                                                  "for the given QMake project.\n\n",
                                      epilog="Please emsure that the proper VS build environment is sourced before running this tool. "
                                             "Options marked with [formattable] can be specified with new-style named Python "
                                             "formatters (e.g. {var1}), and inputs to those formatters can be specified via --fmt. "
                                             "Automatically provided variables (but overridable with --fmt as needed) include: qt_version",
                                      formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                      default_config_files=["qt_dev_archive.cfg"])
    parser_group_cfg = parser.add_argument_group("Configuration options")
    parser_group_cfg.add_argument("--config",
                                  is_config_file=True,
                                  help="config file path")
    parser_group_cfg.add_argument("--print-config",
                                  action="store_true",
                                  help="print out configuration loaded in, including CLI args")
    parser_group_cfg.add_argument("--write-config",
                                  is_write_out_config_file_arg=True,
                                  help="write out config file based on all configs loaded in, including CLI args")
    
    parser_group_tools = parser.add_argument_group("Tools options")
    parser_group_tools.add_argument("--7-zip",
                                    dest="seven_zip",
                                    default=DEFAULT_SEVENZIP,
                                    help="path to 7-zip's 7z.exe [formattable]")
    
    parser_group_qt = parser.add_argument_group("Qt options")
    parser_group_qt.add_argument("--qt-base-dir",
                                 default=DEFAULT_QT_BASE_DIR,
                                 help="Qt base dir to pull Qt installations from (relative to --root-dir) [formattable]")
    parser_group_qt.add_argument("--qt-version",
                                 required=True,
                                 help="Qt version to archive [formattable]")
    parser_group_qt.add_argument("--qt-dir",
                                 default=DEFAULT_QT_DIR,
                                 help="actual Qt installation dir name [formattable]")
    parser_group_qt.add_argument("--qt-lib-include",
                                 action="append",
                                 default=DEFAULT_QT_LIB_INCLUDE,
                                 help="Qt libraries to include")
    parser_group_qt.add_argument("--qt-lib-dll-subdir",
                                 default=DEFAULT_QT_LIB_DLL_SUBDIR,
                                 help="subdirectory to search in for DLLs; empty to not use a subdirectory [formattable]")
    
    parser_group_archive_path_gen = parser.add_argument_group("Archive, path, and general options")
    parser_group_archive_path_gen.add_argument("--root-dir",
                                               default=DEFAULT_ROOT_DIR,
                                               help="root dir to cd into and archive from - applicable to both Qt and LibreSSL dirs [formattable]")
    parser_group_archive_path_gen.add_argument("--archive-exclude",
                                               action="append",
                                               default=DEFAULT_ARCHIVE_EXCLUDE,
                                               help="patterns to exclude from the archive - applies to Qt and all other included items; "
                                                    "do not change unless you know what you are doing!")
    parser_group_archive_path_gen.add_argument("--addl-path",
                                               action="append",
                                               help="additional paths (relative to --root-dir) to add to the archive - useful if "
                                                    "dependencies to Qt and/or the app are required, such as SSL libraries "
                                                    "[formattable]")
    parser_group_archive_path_gen.add_argument("--output-archive",
                                               default=DEFAULT_OUTPUT_ARCHIVE,
                                               help="output archive path - should end with 7z [formattable]")
    parser_group_archive_path_gen.add_argument("--fmt",
                                               nargs=1,
                                               action="append",
                                               metavar="NAME=VALUE",
                                               help="add name=value formatters to apply on various arguments specified")
    parser_group_archive_path_gen.add_argument("--log-file",
                                               help="save log to file (and suppress terminal output) [formattable]")
    parser_group_archive_path_gen.add_argument("--verbose",
                                               action="store_true",
                                               help="enable verbose printing (mainly debug log)")
    
    args = parser.parse_args()
    
    # This has to be done separately, otherwise configargparse doesn't like it
    args.fmt = parse_kv_to_dict(args.fmt) if args.fmt else args.fmt
    
    return args, parser


def format_until_done(text, **fmt):
    last_fmt = text
    while 1:
        next_fmt = last_fmt.format(**fmt)
        if next_fmt == last_fmt:
            break
        last_fmt = next_fmt
    return last_fmt

def format_args(args):
    EXCLUDED_FMT_ARGS = ["qt_lib_include", "config", "write_config"]
    fmted_args = argparse.Namespace()
    
    fmt = {
        "qt_version": args.qt_version
    }
    if args.fmt:
        fmt.update(args.fmt)
    
    # preprocess qt_version
    fmt["qt_version"] = format_until_done(fmt["qt_version"], **fmt)
    
    for arg in vars(args):
        arg_val = getattr(args, arg)
        
        if arg in EXCLUDED_FMT_ARGS or arg == "fmt":
            setattr(fmted_args, arg, arg_val)
            continue
        
        arg_val_xformed = None
        if fmt:
            if type(arg_val) in (str, bytes):
                arg_val_xformed = format_until_done(arg_val, **fmt)
            elif type(arg_val) in (tuple, list):
                arg_val_xformed = [format_until_done(v, **fmt) for v in arg_val]
                arg_val_xformed = type(arg_val)(arg_val_xformed)
            else:
                arg_val_xformed = arg_val
        else:
            arg_val_xformed = arg_val
        setattr(fmted_args, arg, arg_val_xformed)
    return fmted_args

# Don't change this - we need this to check
WINDOW_CMDLINE_LIMIT = 32767

def subprocess_call(*args, **kwargs):
    string_cmd = " ".join(args[0])

    logging.debug("Running command: %s (%s chars)", string_cmd, len(" ".join(args[0])))
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
        logging.error("   -> ERROR: File (prefix) %s doesn't exist!", filename)
        sys.exit(1)
    
    if len(file_list) == 1:
        logging.info("   -> Renaming: %s -> %s (only one part detected)", os.path.basename(file_list[0]), basename_fn)
        os.rename(file_list[0], filename)
    else:
        logging.info("   -> Not renaming: %s (found %i parts!)", os.path.basename(file_list[0]), len(file_list))

def silent_remove_wildcard(file_wc):
    for f in glob.glob(file_wc):
        silentremove(f)

def format_banner(text):
    middle = "= {} =".format(text)
    top_bottom = "=" * len(middle)
    return f"{top_bottom}\n{middle}\n{top_bottom}"

def print_banner(text):
    banner = format_banner(text)
    for l in banner.split("\n"):
        logging.info(l)

def initialize_env():
    # Modify PATH if needed
    entire_path = os.environ["PATH"]
    entire_path = entire_path.split(os.pathsep)
    entire_path = [p for p in entire_path if "PyQt5" not in p]
    entire_path = os.pathsep.join(entire_path)
    os.environ["PATH"] = entire_path

def cleanup_old_archive(output_archive):
    return silent_remove_wildcard(output_archive)

def run_qmake(root_dir, qt_base_dir, qt_dir, project_path):
    os.chdir(os.path.dirname(os.path.realpath(__file__)))
    mkdir_p("build_chk")

    os.chdir("build_chk")
    logging.debug("Current CWD: %s", os.getcwd())

    REQUIRED_ENV_VARS = [
        ["ZLIB_LIB", "LIBPNG_APNG_LIB", "LIBPNG_APNG_INCLUDE"],
        ["LIBPNG_APNG_FROM_VCPKG"]
    ]

    if not any(all([True if p in os.environ else False for p in envset]) for envset in REQUIRED_ENV_VARS):
        logging.error(" ! ERROR: These environment variables for 32-bit builds must be defined for detection to work: %s",
                      " OR ".join("(%s)" % ", ".join(envset) for envset in REQUIRED_ENV_VARS))
        sys.exit(1)

    qmake_path = os.path.join(root_dir, qt_base_dir, qt_dir, "bin", "qmake")

    if "LIBPNG_APNG_LIB" in os.environ:
        if not simple_exec([qmake_path, '-spec', 'win32-msvc', '-tp', 'vc',
                           'LIBS+=' + os.environ.get('ZLIB_LIB'), 'LIBS+=' + os.environ.get('LIBPNG_APNG_LIB'),
                           'INCLUDEPATH+=' + os.environ.get('LIBPNG_APNG_INCLUDE'), project_path]):
            logging.error(" ! ERROR: Creating project files for x86 failed (using LIBPNG_APNG_LIB env)!")
            sys.exit(1)
    else:
        if not simple_exec([qmake_path, '-spec', 'win32-msvc', '-tp', 'vc',
                            'LIBPNG_APNG_FROM_VCPKG=' + os.environ.get('LIBPNG_APNG_FROM_VCPKG'),
                             project_path]):
            logging.error(" ! ERROR: Creating project files for x86 failed (using LIBPNG_APNG_FROM_VCPKG env)!")
            sys.exit(1)
    os.chdir("..")

def build_exclusion_list(root_dir, qt_base_dir, qt_dir, qt_lib_include, found_libs, lib_dll_subdir=""):
    # First, find all of the DLL files
    lib_matches = []
    pdb_matches = []

    # Is it bad to just base everything off of 32-bit? Assuming libraries
    # have the same version, maybe not...
    
    qt_lib_dll_dir = os.path.join(root_dir, qt_base_dir, qt_dir, lib_dll_subdir)

    for root, dirnames, filenames in os.walk(qt_lib_dll_dir):
        for filename in fnmatch.filter(filenames, '*.lib'):
            lib_matches.append(os.path.join(root, filename))

    for root, dirnames, filenames in os.walk(qt_lib_dll_dir):
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

    qt_lib_include_arr = [ ("qt5" + lib.lower()) for lib in qt_lib_include ] + found_libs

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
            logging.debug("%s vs %s = %.2f", first_prefix_match, second_prefix_match, similarity)
            if similarity < 0.5:
                prefix_matches_j -= 1
                break
            
            prefix_matches_j += 1
        
        possible_common_prefix_matches = prefix_matches[prefix_matches_i:prefix_matches_j+1]
        
        if len(possible_common_prefix_matches) > 1:
            common_prefix = os.path.commonprefix(possible_common_prefix_matches)
            common_suffix = commonsuffix(possible_common_prefix_matches)
            
            logging.debug("Checking: %s", ", ".join(possible_common_prefix_matches))
            logging.debug("Common prefix: %s", common_prefix)
            logging.debug("Common suffix: %s", common_suffix)
            
            possible_wcs.add((common_prefix, common_suffix))
            elements_wcs.update(possible_common_prefix_matches)
            
            prefix_matches_i = prefix_matches_j
        
        prefix_matches_i += 1

    non_wcs = set(prefix_matches).difference(elements_wcs)
    final_wcs = set()

    logging.debug("non_wcs = %s", non_wcs)
    logging.debug("qt_lib_include_arr = %s", qt_lib_include_arr)

    lc_prefix_matches = [pm.lower() for pm in prefix_matches]

    for possible_wc_tuple in possible_wcs:
        possible_wc = "{0}*{1}".format(*possible_wc_tuple)
        logging.debug(possible_wc)
        too_broad = False
        for lib_match in qt_lib_include_arr + [qlia + "d" for qlia in qt_lib_include_arr]:
            if fnmatch.fnmatch(lib_match.lower(), possible_wc.lower()) and lib_match not in lc_prefix_matches:
                logging.debug("too broad, removing (%s matched by wc %s)", lib_match, possible_wc)
                too_broad = True
                break
        if not too_broad:
            final_wcs.add(possible_wc)

    logging.debug("final_wcs = %s", final_wcs)

    logging.debug("** pre prefix_matches = %s", prefix_matches)
    pre_prefix_matches_len = len(prefix_matches)

    prefix_matches = [prefix_match for prefix_match in prefix_matches
                      if not any(fnmatch.fnmatch(prefix_match, final_wc) for final_wc in final_wcs)]
    prefix_matches += final_wcs

    logging.debug("** post prefix_matches = %s", prefix_matches)

    logging.info("   -- Using %d specific wildcards, transformed %d excludes to %d excludes.", len(final_wcs), pre_prefix_matches_len, len(prefix_matches))
    
    return prefix_matches

def generate_exclusion_flags_from_exclusion_prefix_matches(qt_lib_include, archive_excludes, prefix_matches):
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

    if (not ("webengine" in qt_lib_include)) and (not ("webenginecore" in qt_lib_include)):
        all_excludes.append("-xr!qtwebengine*")

    all_excludes = all_excludes + ["-xr!" + e for e in archive_excludes]
    
    return all_excludes

def create_archive(root_dir, qt_base_dir, qt_dir, seven_zip_exe, output_archive, addl_paths=None, addl_flags=None, lib_dll_subdir=""):
    # To preserve path in 7-Zip, you MUST change directory to where the
    # folder is. If you don't, the first part of the path will be lost.
    os.chdir(root_dir)
    
    qt_path = os.path.join(qt_base_dir, qt_dir)

    addl_paths = addl_paths or []
    addl_flags = addl_flags or []

    # IMPORTANT NOTE: All paths must be in quotes! Otherwise, 7-Zip will
    # NOT parse the paths correctly, and weird things will happen!
    subprocess_call([seven_zip_exe, "a", output_archive] + addl_paths + [qt_path,
                    "-t7z", "-m0=lzma2", "-mx9", "-v100000000",
                        # Exclude unnecessary files
                        
                        # Installation files
                        "-xr!" + os.path.join(qt_path, "network.xml"),
                        "-xr!" + os.path.join(qt_path, "Maintenance*"),
                        "-xr!" + os.path.join(qt_path, "InstallationLog.txt"),
                        "-xr!" + os.path.join(qt_path, "components.xml"),
                        
                        # Tools, examples, documentation, and VCRedist
                        "-xr!" + os.path.join(qt_path, "Tools"),
                        "-xr!" + os.path.join(qt_path, "Examples"),
                        "-xr!" + os.path.join(qt_path, "Docs"),
                        "-xr!" + os.path.join(qt_path, "vcredist"),
                        "-xr!" + os.path.join(qt_path, lib_dll_subdir, "doc"),
                        
                        # Note that we can't just delete all debug DLLs,
                        # LIBs, or PDBs, since they are required to compile
                        # debug builds. We have to find the libraries we
                        # need, then exclude the rest - the technique done
                        # above.
                        #"-xr!*d.dll",
                        #"-xr!*d.lib",
                        #"-xr!*.pdb",
                    ] + addl_flags)

    # -xr!*d.dll -xr!*d.lib -xr!*.pdb

    move_single_7z_file(output_archive)

def build_archive(root_dir, qt_base_dir, qt_dir, qt_version, qt_lib_include, archive_excludes, output_archive, seven_zip_exe, addl_paths=None, lib_dll_subdir=""):
    print_banner(f"Building Qt v{qt_version} development archive")

    initialize_env()

    cdir = os.getcwd()
    output_archive = os.path.join(cdir, output_archive)

    logging.info(" * Stage 0: Removing Old Archives")
    cleanup_old_archive(output_archive)

    logging.info(" * Stage 1: Analyzing CEmu")
    run_qmake(root_dir, qt_base_dir, qt_dir, r'..\..\CEmu.pro')

    logging.info(" * Stage 2: Building Exclusion List")

    logging.info("   -> Stage 2a: Collecting Qt Dependencies")
    cemu32_proj = os.path.join("build_chk", "CEmu.vcxproj")

    found_libs = collect_qt_static_files(cemu32_proj)

    logging.info("   -- Found %d Qt libraries to include.", len(found_libs))

    logging.info("   -> Stage 2b: Creating Exclusion List")
    exclusion_prefix_matches = build_exclusion_list(root_dir, qt_base_dir, qt_dir, qt_lib_include, found_libs, lib_dll_subdir=lib_dll_subdir)
    all_exclude_flags = generate_exclusion_flags_from_exclusion_prefix_matches(qt_lib_include, archive_excludes, exclusion_prefix_matches)
    
    logging.info("   -- Excluding %d files/wildcards from the archive.", len(all_exclude_flags))

    shutil.rmtree("build_chk")

    logging.info(" * Stage 3: Building Development Archive")
    create_archive(root_dir, qt_base_dir, qt_dir, seven_zip_exe, output_archive,
                   addl_paths=addl_paths, addl_flags=all_exclude_flags, lib_dll_subdir=lib_dll_subdir)

    logging.info(" * All done!")

if __name__ == "__main__":
    args, parser = parse_args()
    fmted_args = format_args(args)
    
    logging.basicConfig(
        filename=fmted_args.log_file,
        level=logging.DEBUG if fmted_args.verbose else logging.INFO,
        format= '[%(asctime)s] {%(module)s::%(funcName)-25s/%(pathname)s:%(lineno)d} %(levelname)7s - %(message)s',
        datefmt='%Y%m%d %H:%M:%S'
    )
    
    if args.print_config:
        logging.info("Displaying resulting config...\n%s", parser.format_values()) 
        logging.info("After applying formatting, resulting runtime config:\n%s",
            "\n".join(["{} = {}".format(arg, getattr(fmted_args, arg)) for arg in vars(fmted_args)]))
        sys.exit(0)
    
    build_archive(fmted_args.root_dir, fmted_args.qt_base_dir, fmted_args.qt_dir, fmted_args.qt_version, fmted_args.qt_lib_include, fmted_args.archive_exclude, fmted_args.output_archive,
                  fmted_args.seven_zip, addl_paths=fmted_args.addl_path, lib_dll_subdir=fmted_args.qt_lib_dll_subdir)