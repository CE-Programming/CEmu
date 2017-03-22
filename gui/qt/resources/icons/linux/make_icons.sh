#!/bin/bash
# Script to generate Linux icons from SVG
for length in 512 256 192 160 128 96 72 64 48 42 40 36 32 24 22 20 16; do
    echo "Creating icon: $length"x"$length"
    inkscape -z -e cemu-"$length"x"$length".png -w $length -h $length ../icon.svg
done
