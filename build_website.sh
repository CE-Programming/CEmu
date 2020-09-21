#!/bin/bash
# Just build the website.
# 

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
echo "Started build..."
mkdir html

# Build site.
echo " -> Building website..."
pip3 install -r requirements.txt; cerr $? "Python Pip Requirements Installation"
python3 generate.py; cerr $? "Webpage Generation"

cp -r css fonts img js html/; cerr $? "Copying Webpage Support Files"

echo "Website built successfully!"
exit 0
