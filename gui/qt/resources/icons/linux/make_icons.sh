#!/bin/bash
# Script to generate Linux icons from SVG.
#
# Requires Inkscape to convert from SVG to PNG.
#
# To enable PNG compression, install and/or place supported PNG
# compression tools here. If multiple tools are found, this script
# will run the tool with better compression.
#
# Supported tools (in order of lossless compression savings):
#   pingo: https://css-ig.net/pingo
#     (requires WINE to run, download and save EXE in this directory)
#   zopflipng: https://github.com/google/zopfli
#   pngout: http://www.jonof.id.au/kenutils
#   advpng: https://github.com/amadvance/advancecomp
#

becho() { echo -e "\e[1m$@\e[0m"; }

for length in 512 256 192 160 128 96 72 64 48 42 40 36 32 24 22 20 16; do
    becho "Creating icon: $length"x"$length"
    inkscape -z -e cemu-"$length"x"$length".png -w $length -h $length ../icon.svg

    # PNG compression tools (lossless)
    wine pingo -s4 cemu-"$length"x"$length".png 2>/dev/null && \
        # Yes, you can run pingo twice and still save some bytes!
        wine pingo -s4 cemu-"$length"x"$length".png 2>/dev/null && \
        becho ' -> Compressed w/pingo!' && continue
    zopflipng -y -m cemu-"$length"x"$length".png cemu-"$length"x"$length".png 2>/dev/null && \
        becho ' -> Compressed w/zopflipng!' && continue
    pngout-static cemu-"$length"x"$length".png 2>/dev/null && \
        becho ' -> Compressed w/pngout!' && continue
    advpng -z -4 cemu-"$length"x"$length".png 2>/dev/null && \
        becho ' -> Compressed w/advpng!' || \
        # Warn user about lack of compression!
        becho ' -> WARN: No compression tools found! PNG will not be compressed.'
done
