import jinja2
import os
import datetime

base_dir = os.path.dirname(os.path.abspath(__file__))

templateLoader = jinja2.FileSystemLoader( searchpath=os.path.join(base_dir, "templates"))
templateEnv = jinja2.Environment(loader=templateLoader, lstrip_blocks=True, trim_blocks=True)

def get_git_info():
    try:
        commit_hash = subprocess.check_output(['git', 'rev-parse', '--verify', 'HEAD'])
    except:
        commit_hash = None
    
    try:
        repo_url = subprocess.check_output(['git', 'config', 'remote.origin.url'])
    except:
        repo_url = None
    
    return commit_hash, repo_url

def generate_file(file_name):
    base_name = file_name
    file_name = file_name + ".html"
    
    # Define this if the page is located in a subdirectory instead
    # of the domain root.
    page_subdir = "/CEmu"

    template = templateEnv.get_template( file_name )
    
    commit_hash, repo_url = get_git_info()
    
    # this is where to put args to the template renderer
    outputText = template.render(active_page = base_name,
                                 page_subdir = page_subdir,
                                 build_time  = str(datetime.datetime.now()),
                                 commit_hash = commit_hash,
                                 repo_url    = repo_url) 

    file = open(os.path.join(base_dir, "html", file_name), 'w')
    file.write(outputText)

    print("Rendered", file_name)

files = ['404', 'index', 'download']

if not os.path.isdir(os.path.join(base_dir, "html")):
    os.mkdir(os.path.join(base_dir, "html"))

for file in files:
    generate_file(file)
