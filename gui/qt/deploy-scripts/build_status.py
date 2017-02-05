#!/usr/bin/env python
import ircmsgbot
import os
import sys
import subprocess
from jinja2 import Template
import logging

try:
    # Python 3
    from urllib.request import urlopen, Request
    from urllib.parse import urlencode
    from urllib.error import HTTPError, URLError
    from http.client import HTTPException
except ImportError:
    # Python 2
    from urllib2 import urlopen, urlencode, Request, HTTPError, URLError
    from httplib import HTTPException

IRC_SERVER = "irc.efnet.org"
IRC_PORT   = 6667
IRC_NICK   = "CEmuMSVC"
IRC_TARGET = '#thebot'
IRC_USESSL = False

# Add a blank line to indicate a new message.
irc_build_msgs = """
[
{{ color }}{{ purple }}
{{ repo_name }}/{{ branch }}
 
(
{{ commit_hash_short }}, 
{{ repo_url_tiny }}
{% if is_tag %}
, tag {{ tag_name }}
{% endif %}
)
{{ endcolor }}
]
 
{% if build_passed %}
{{ color }}{{ green }}{{ build_pass_symbol }} Build passed!{{ endcolor }}
{% else %}
{{ color }}{{ red }}{{ build_fail_symbol }} Build failed!{{ endcolor }}
{% endif %}
 {{ appveyor_build_url_tiny }}
{% if is_scheduled_build %}
 (Scheduled Build)
{% elif is_forced_build %}
 (Forced Build)
{% elif is_rebuild %}
 (Rebuild)
{% endif %}
 
(
{{ author_name }},
 
{{ commit_msg }}
)
"""

started_build_msgs = """
[
{{ color }}{{ purple }}
{{ repo_name }}/{{ branch }}
 
(
{{ commit_hash_short }}, 
{{ repo_url_tiny }}
{% if is_tag %}
, tag {{ tag_name }}
{% endif %}
)
{{ endcolor }}
]
 
{{ color }}{{ orange }}
Build started @ {{ appveyor_build_url_tiny }}!
{{ endcolor }}
{% if is_scheduled_build %}
 (Scheduled Build)
{% elif is_forced_build %}
 (Forced Build)
{% elif is_rebuild %}
 (Rebuild)
{% endif %}
 
(
{{ author_name }},
 
{{ commit_msg }}
)
"""

# Prefunctions
def shorten_url_gitio(url, alt = None):
    data = { 'url' : url }
    form_data = urlencode(data).encode("utf-8")
    
    try:
        out = urlopen("https://git.io", form_data)
        res = dict(out.info()).get('Location')
        if res == None:
            return alt
        return res
    # Error handling...
    except HTTPError:
        _, e, _ = sys.exc_info()
        print(" ! HTTP Error: %i (%s)" % (e.code, url))
        print(" ! Server said:")
        err = e.read().decode("utf-8")
        err = " !   " + "\n !   ".join(err.split("\n")).strip()
        print(err)
    except URLError:
        _, e, _ = sys.exc_info()
        print(" ! URL Error: %s (%s)" % (e.reason, url))
    except HTTPException:
        _, e, _ = sys.exc_info()
        print(" ! HTTP Exception: %s (%s)" % (str(e), url))
    
    return alt

def shorten_url_google(url, alt = None):
    # A Google API key is necessary to make this function work reliably.
    # 
    # You can fetch one via the Google Developer Console:
    # https://console.developers.google.com/
    # 
    # First, enable the URL shortener API. Then, add a new API key to
    # use. For maximum security, you MUST restrict the API key by
    # AppVeyor IPs. Those IPs can be found here:
    # https://www.appveyor.com/docs/build-configuration/#ip-addresses
    # 
    # Finally, you will need to encrypt your API key with AppVeyor, and
    # add it to your appveyor.yml. You can encrypt it here:
    # https://ci.appveyor.com/tools/encrypt
    # 
    # Add the encrypted API key as environment variable
    # GOOGLE_URL_SHORTENER_API_KEY to your appveyor.yml, and you should
    # be set!
    
    # Attempt to locate API key
    google_url_shortener_api_key = os.environ.get("GOOGLE_URL_SHORTENER_API_KEY")
    if google_url_shortener_api_key:
        api_url = "https://www.googleapis.com/urlshortener/v1/url?key=%s" % google_url_shortener_api_key
    else:
        print(" ! WARNING: No Google API key found. You should strongly consider")
        print(" !          adding one - read shorten_url_google() documentation")
        print(" !          for more information.")
        api_url = "https://www.googleapis.com/urlshortener/v1/url"
    
    headers = { "Content-Type": "application/json" }
    data = '{"longUrl": "%s"}' % url
    form_data = data.encode("utf-8")
    
    res = None
    
    try:
        req = Request(api_url, form_data, headers = headers)
        out = urlopen(req)
        res = out.read()
        short_url = json.loads(out.read()).get("id")
        if short_url == None:
            return alt
        return short_url
    # Error handling...
    except HTTPError:
        _, e, _ = sys.exc_info()
        print(" ! HTTP Error: %i (%s)" % (e.code, url))
        print(" ! Server said:")
        err = e.read().decode("utf-8")
        err = " !   " + "\n !   ".join(err.split("\n")).strip()
        print(err)
    except URLError:
        _, e, _ = sys.exc_info()
        print(" ! URL Error: %s (%s)" % (e.reason, url))
    except HTTPException:
        _, e, _ = sys.exc_info()
        print(" ! HTTP Exception: %s (%s)" % (str(e), url))
    except ValueError:
        _, e, _ = sys.exc_info()
        print(" ! JSON Exception: %s (%s)" % (str(e), url))
    
    return alt

irc_format_table = {
    "white"       : "00",
    "black"       : "01",
    "blue"        : "02",
    "green"       : "03",
    "red"         : "04",
    "brown"       : "05",
    "purple"      : "06",
    "orange"      : "07",
    "yellow"      : "08",
    "light_green" : "09",
    "teal"        : "10",
    "light_cyan"  : "11",
    "light_blue"  : "12",
    "pink"        : "13",
    "grey"        : "14",
    "light_grey"  : "15",
    "default"     : "99",
    "color"       : "\x03",
    "endcolor"    : "\x03",
    "bold"        : "\x02",
    "italic"      : "\x1D",
    "underline"   : "\x1F",
    "invert"      : "\x16",
    "reset"       : "\x0F",
}

appveyor_env = {
    "appveyor_enabled"                      : os.environ.get("APPVEYOR"),                                    # True if build runs in AppVeyor environment;
    "ci_enabled"                            : os.environ.get("CI"),                                          # True if build runs in AppVeyor environment;
    "appveyor_api_url"                      : os.environ.get("APPVEYOR_API_URL"),                            # AppVeyor Build Agent API URL;
    "appveyor_acct_name"                    : os.environ.get("APPVEYOR_ACCOUNT_NAME"),                       # account name;
    "appveyor_proj_id"                      : os.environ.get("APPVEYOR_PROJECT_ID"),                         # AppVeyor unique project ID;
    "appveyor_proj_name"                    : os.environ.get("APPVEYOR_PROJECT_NAME"),                       # project name;
    "appveyor_proj_slug"                    : os.environ.get("APPVEYOR_PROJECT_SLUG"),                       # project slug (as seen in project details URL);
    "build_folder"                          : os.environ.get("APPVEYOR_BUILD_FOLDER"),                       # path to clone directory;
    "build_id"                              : os.environ.get("APPVEYOR_BUILD_ID"),                           # AppVeyor unique build ID;
    "build_num"                             : os.environ.get("APPVEYOR_BUILD_NUMBER"),                       # build number;
    "build_ver"                             : os.environ.get("APPVEYOR_BUILD_VERSION"),                      # build version;
    "pr_num"                                : os.environ.get("APPVEYOR_PULL_REQUEST_NUMBER"),                # GitHub Pull Request number;
    "pr_title"                              : os.environ.get("APPVEYOR_PULL_REQUEST_TITLE"),                 # GitHub Pull Request title
    "job_id"                                : os.environ.get("APPVEYOR_JOB_ID"),                             # AppVeyor unique job ID;
    "job_name"                              : os.environ.get("APPVEYOR_JOB_NAME"),                           # job name;
    "repo_provider"                         : os.environ.get("APPVEYOR_REPO_PROVIDER"),                      # github, bitbucket, kiln, vso or gitlab;
    "scm"                                   : os.environ.get("APPVEYOR_REPO_SCM"),                           # git or mercurial;
    "name"                                  : os.environ.get("APPVEYOR_REPO_NAME"),                          # repository name in format owner-name/repo-name;
    "branch"                                : os.environ.get("APPVEYOR_REPO_BRANCH"),                        # build branch. For Pull Request commits it is base branch PR is merging into;
    "is_tag"                                : os.environ.get("APPVEYOR_REPO_TAG"),                           # true if build has started by pushed tag; otherwise false;
    "tag_name"                              : os.environ.get("APPVEYOR_REPO_TAG_NAME"),                      # contains tag name for builds started by tag; otherwise this variable is undefined;
    "commit_hash"                           : os.environ.get("APPVEYOR_REPO_COMMIT"),                        # commit ID (SHA);
    "author_name"                           : os.environ.get("APPVEYOR_REPO_COMMIT_AUTHOR"),                 # commit author’s name;
    "author_email"                          : os.environ.get("APPVEYOR_REPO_COMMIT_AUTHOR_EMAIL"),           # commit author’s email address;
    "commit_timestamp"                      : os.environ.get("APPVEYOR_REPO_COMMIT_TIMESTAMP")[:19] if (     # commit date/time;
                                                os.environ.get("APPVEYOR_REPO_COMMIT_TIMESTAMP")) else "--", # 
    "commit_msg"                            : os.environ.get("APPVEYOR_REPO_COMMIT_MESSAGE"),                # commit message;
    "commit_msg_long"                       : os.environ.get("APPVEYOR_REPO_COMMIT_MESSAGE_EXTENDED"),       # the rest of commit message after line break (if exists);
    "is_scheduled_build"                    : os.environ.get("APPVEYOR_SCHEDULED_BUILD"),                    # True if the build runs by scheduler;
    "is_forced_build"                       : os.environ.get("APPVEYOR_FORCED_BUILD"),                       # (True or undefined) - builds started by “New build” button or from the same API;
    "is_rebuild"                            : os.environ.get("APPVEYOR_RE_BUILD"),                           # (True or undefined) - build started by “Re-build commit/PR” button of from the same API;
    "appveyor_url"                          : os.environ.get("APPVEYOR_URL"),                                # AppVeyor base URL - undocumented
    "platform_name"                         : os.environ.get("PLATFORM"),                                    # platform name set on Build tab of project settings (or through platform parameter in appveyor.yml);
    "config_name"                           : os.environ.get("CONFIGURATION"),                               # configuration name set on Build tab of project settings (or through configuration parameter in appveyor.yml);
    "artifact_upload_timeout"               : os.environ.get("APPVEYOR_ARTIFACT_UPLOAD_TIMEOUT"),            # artifact upload timeout in seconds. Default is 600 (10 minutes);
    "file_download_timeout"                 : os.environ.get("APPVEYOR_FILE_DOWNLOAD_TIMEOUT"),              # timeout in seconds to download arbirtary files using appveyor DownloadFile command. Default is 300 (5 minutes); 
    "repo_shallow_clone_timeout"            : os.environ.get("APPVEYOR_REPOSITORY_SHALLOW_CLONE_TIMEOUT"),   # timeout in seconds to download repository (GitHub, Bitbucket or VSTS) as zip file (shallow clone). Default is 1800 (30 minutes);
    "cache_entry_upload_download_timeout"   : os.environ.get("APPVEYOR_CACHE_ENTRY_UPLOAD_DOWNLOAD_TIMEOUT"),# timeout in seconds to download or upload each cache entry. Default is 300 (5 minutes);
}

extra_env = {
    "appveyor_proj_url"                     : "%s/project/%s/%s" % (                                         # AppVeyor project URL
                                                    appveyor_env["appveyor_url"],                            # Example:
                                                    appveyor_env["appveyor_acct_name"],                      #   https://ci.appveyor.com/project/alberthdev/cemu-q0nl8
                                                    appveyor_env["appveyor_proj_slug"],
                                                ),
    "appveyor_build_url"                    : "%s/project/%s/%s/build/%s" % (                                # AppVeyor build URL
                                                    appveyor_env["appveyor_url"],                            # Example:
                                                    appveyor_env["appveyor_acct_name"],                      #   https://ci.appveyor.com/project/alberthdev/cemu-q0nl8/build/1.0.1
                                                    appveyor_env["appveyor_proj_slug"],
                                                    appveyor_env["build_ver"],
                                                ),
    "build_pass_symbol"                     : "✓",
    "build_fail_symbol"                     : "✗",
    "commit_hash_short"                     : appveyor_env["commit_hash"][:7] if (
                                                appveyor_env["commit_hash"]) else "--",
    "repo_name"                             : appveyor_env["name"].split("/")[-1] if (
                                                appveyor_env["name"]) else "--",
}

extra_env["repo_url"]                       = "https://%s.com/%s/%s/%s" % (                        # Repository URL
                                                  appveyor_env["repo_provider"],                   # Example:
                                                  appveyor_env["name"],                            #   https://github.com/CE-Programming/CEmu/commit/4603aec71f9e1163e545beff10122ef40ec9007a
                                                  "commit" if (
                                                      appveyor_env["repo_provider"].lower() in [
                                                          "github", "gitlab"
                                                      ]
                                                  ) else "commits",
                                                  extra_env["commit_hash_short"]
                                              ) if (
                                                  appveyor_env["repo_provider"] and
                                                  appveyor_env["repo_provider"].lower() in [
                                                      "github", "gitlab", "bitbucket"
                                                  ]
                                              ) else "(none)"
extra_env["repo_url_long"]                  = "https://%s.com/%s/%s/%s" % (                        # Repository URL
                                                  appveyor_env["repo_provider"],                   # Example:
                                                  appveyor_env["name"],                            #   https://github.com/CE-Programming/CEmu/commit/4603aec71f9e1163e545beff10122ef40ec9007a
                                                  "commit" if (
                                                      appveyor_env["repo_provider"].lower() in [
                                                          "github", "gitlab"
                                                      ]
                                                  ) else "commits",
                                                  appveyor_env["commit_hash"]
                                              ) if (
                                                  appveyor_env["repo_provider"] and
                                                  appveyor_env["repo_provider"].lower() in [
                                                      "github", "gitlab", "bitbucket"
                                                  ]
                                              ) else "(none)"
extra_env["repo_url_tiny"]                  = shorten_url_gitio(extra_env["repo_url_long"], alt = "(none)") if (
                                                  extra_env["repo_url_long"] != "(none)") else "(none)"
extra_env["appveyor_build_url_tiny"]        = shorten_url_google(extra_env["appveyor_build_url"], alt = "(none)")

bool_vars = [
    "appveyor_enabled",
    "ci_enabled",
    "is_tag",
    "is_scheduled_build",
    "is_forced_build",
    "is_rebuild",
]

for bool_var in bool_vars:
    appveyor_env[bool_var] = True if appveyor_env[bool_var] == "True" else False

# Create final dict
format_dict = { **irc_format_table, **appveyor_env, **extra_env }

def render_status(build_passed = None, msgs = irc_build_msgs, addl_vars = None):
    # Rule:
    # Newlines at the beginning and the end are stripped
    # Blank lines = start a new message
    # Otherwise every new line will be joined together, without anything
    # in between.
    msgs_split = msgs.strip("\n").split("\n")
    msgs_split = [ "\n" if (msg == "") else msg for msg in msgs_split ]
    final_msgs = "".join(msgs_split)
    
    template = Template(final_msgs)
    return template.render(build_passed = build_passed, **format_dict)

def send_build_status(build_passed, msgs = irc_build_msgs, addl_vars = None, process = False):
    irc_rendered_msg = render_status(build_passed, msgs = msgs, addl_vars = addl_vars)
    
    ircmsgbot.async_send_irc_message(IRC_SERVER, IRC_PORT, IRC_NICK,
        IRC_TARGET, irc_rendered_msg, use_ssl=IRC_USESSL,
        process = process)

def start_build_status(msgs = started_build_msgs, addl_vars = None, process = False):
    irc_rendered_msg = render_status(msgs = msgs, addl_vars = addl_vars)
    
    ircmsgbot.async_send_irc_message(IRC_SERVER, IRC_PORT, IRC_NICK,
        IRC_TARGET, irc_rendered_msg, use_ssl=IRC_USESSL,
        process = process)

def finish_build_status(timeout = 180):
    ircmsgbot.async_stop_all(timeout = timeout)

def restart_without_async():
    argv = list(sys.argv)
    if len(argv) == 3:
        argv.pop()
    subprocess.Popen([sys.executable] + argv)

def main():
    #logging.basicConfig(level=logging.DEBUG,
    #    format='[%(asctime)19s] [%(module)s] [%(name)s.%(funcName)s] %(message)s')
    
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print("Usage: build_status.py [SUCCESS|FAILURE|STARTED] <async: YES|NO>")
        print("\nasync: YES to immediately return, NO to wait for completion")
        sys.exit(1)
    
    cmd = sys.argv[1].upper()
    
    if len(sys.argv) == 3:
        async = True if sys.argv[2].upper() == "YES" else False
    else:
        async = False
        
    if async == True:
        print(" * Async detected, backgrounding this process...")
        restart_without_async()
        sys.exit(0)
    
    if cmd == "SUCCESS":
        print(" * Sending successful build IRC notification...")
        send_build_status(True)
    elif cmd == "FAILURE":
        print(" * Sending failed build IRC notification...")
        send_build_status(False)
    elif cmd == "STARTED":
        print(" * Sending build start IRC notification...")
        start_build_status()
    else:
        print("ERROR: Invalid command.")
        sys.exit(1)
    
    print(" * Waiting for IRC notification to send...")
    finish_build_status()
    
    print(" * IRC notification sent!")

if __name__ == "__main__":
    main()
    sys.exit(0)
