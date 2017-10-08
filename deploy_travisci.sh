#!/bin/bash
# Automated deploy script with Travis-CI.

cerr() {
    if [ ! "$1" = "0" ]; then
        echo "ERROR: Previous command exited with code $1."
        echo "       See above for details."
        echo "       Command: ${@:2}"
        exit $1
    fi
}

# Variables
echo "Started deployment..."

# Build site.
echo " -> Building website..."
pip3 install -r requirements.txt; cerr $? "Python Pip Requirements Installation"
python3 generate.py; cerr $? "Webpage Generation"

cp -r css fonts img js html/; cerr $? "Copying Webpage Support Files"

#echo " -> Notifying IRC..."

echo "Website built successfully!"
exit 0
