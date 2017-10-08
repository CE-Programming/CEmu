#!/bin/bash
# Automated deploy script with Travis-CI.
# 
# Steps to make things work:
#   0) Install Travis-CI CLI.
#   1) Generate a SSH key, and save it as id_rsa_travis_deploy_key:
#      ssh-keygen -t rsa -b 4096 -C "CEmuTravisCI@CE-Programming"
#   2) Run as needed: travis login
#   3) Run: travis encrypt-file id_rsa_travis_deploy_key
#      Make sure to save the output!
#   4) Copy and paste the decryption line into your .travis.yml.
#      It should be placed underneath your before_install, probably
#      as a first step.
#      
#      Once copied over, add "deploy/" before your id_rsa_... path.
#      
#      Once done, it should look something like this:
#      
#      before_install:
#        - openssl aes-256-cbc -K $encrypted_KEY_ID_key -iv $encrypted_KEY_ID_iv -in deploy/id_rsa_travis_deploy_key.enc -out deploy/id_rsa_travis_deploy_key -d
#   5) Move your id_rsa_travis_deploy_key* files to a safe place OUTSIDE
#      of your Git repo folder. That way, you will reduce the change of
#      pushing a private key into the repo.
#   6) Run as needed: mkdir -p deploy
#   7) Run: cp path/to/my/keys/folder/id_rsa_travis_deploy_key.enc deploy/
#   8) Add the updated deploy/id_rsa_travis_deploy_key.enc and
#      .travis.yml to your commit, and push!

cerr() {
    if [ ! "$1" = "0" ]; then
        echo "ERROR: Previous command exited with code $1."
        echo "       See above for details."
        echo "       Command: ${@:2}"
        exit $1
    fi
}

# Variables
TARGET_BRANCH="gh-pages"

REPO=`git config remote.origin.url`
SSH_REPO=${REPO/https:\/\/github.com\//git@github.com:}
SHA=`git rev-parse --verify HEAD`

# Main body
echo "Started deployment..."

# Build site.
echo " -> Building website..."
pip3 install -r requirements.txt; cerr $? "Python Pip Requirements Installation"
python3 generate.py; cerr $? "Webpage Generation"

cp -r css fonts img js html/; cerr $? "Copying Webpage Support Files"

#echo " -> Notifying IRC..."

echo " -> Loading deploy key..."
eval `ssh-agent -s`
ssh-add deploy/id_rsa_travis_deploy_key

echo " -> Starting deployment..."
cd html; cerr $? "Changing to HTML deployment directory"
git config user.name "Travis CI"; cerr $? "Configuring Git username"
git config user.email "CEmuTravisCI@CE-Programming"; cerr $? "Configuring Git email"

# If there are no changes to the compiled out (e.g. this is a README update) then just bail.
if git diff --quiet; then
    echo "No changes to the output on this push; exiting."
    echo " -> Cleaning up a bit..."
    rm deploy/id_rsa_travis_deploy_key; cerr $? "Removing deployment key"
    exit 0
fi

# Commit the "changes", i.e. the new version.
# The delta will show diffs between new and old versions.
git add -A .; cerr $? "Add New HTML Files"
git commit -m "Deploy to GitHub Pages: ${SHA}"; cerr $? "Creating gh-pages branch git commit"
git push $SSH_REPO $TARGET_BRANCH; cerr $? "Pushing to repo"

echo " -> Cleaning up a bit..."
rm deploy/id_rsa_travis_deploy_key; cerr $? "Removing deployment key"

echo "Website built successfully!"
exit 0
