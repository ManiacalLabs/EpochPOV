#must install Pillow (replacement for PIL) http://python-imaging.github.io/
#pip install Pillow
#if needed...
#install setuptools from here: http://www.lfd.uci.edu/~gohlke/pythonlibs/#setuptools
#install pip from here: http://www.lfd.uci.edu/~gohlke/pythonlibs/#pip
#Need to have VS2008 or higher installed on windows for this to work
#if higher than 2008, run one of the following commands at the console
#VS2010:
#SET VS90COMNTOOLS=%VS100COMNTOOLS%
#VS2012
#SET VS90COMNTOOLS=%VS110COMNTOOLS%
#VS2013
#SET VS90COMNTOOLS=%VS120COMNTOOLS%
from PIL import Image
import sys
import argparse
import struct
import os

def genHeader(cols, delay):
    output = ""
    output += "PROGMEM uint32_t imageData[] = {\n"
    for c in cols:
        print c
        print "0x%0.8X" % c
        output += "\t0x%0.8X,\n" % c

    output += "};\n"
    output += "const uint8_t imageSize = %d;\n" % len(cols)
    output += "const uint8_t frameDelay = %d;\n" % delay
    return output

def printCols(cols):
    for c in cols:
        for y in range(32):
            if c & (1 << y) > 0:
                sys.stdout.write("X")
            else:
                sys.stdout.write(".")
        print ""

_invert = False
_flip = False
_upload = False
_filename = ""
_header = ""
_delay = 1
_clear = False


parser = argparse.ArgumentParser()
parser.add_argument("input_img", help="BMP image to process. Must be 32 pixels high.")
parser.add_argument("output_header", help="File to output C image data to.")
parser.add_argument("--flip_v", help="Flip image vertically.", action="store_true")
parser.add_argument("--flip_h", help="Flip image horizontally.", action="store_true")
parser.add_argument("--invert", help="Invert image black/white values (negative)", action="store_true")

args = parser.parse_args()

_filename = args.input_img
_header = args.output_header
_flip_v = args.flip_v
_flip_h = args.flip_h
_invert = args.invert

cols = []

print "Loading Image: " + _filename
if _filename.endswith(".bmp") != True:
    print "File MUST be a .bmp file!"
    sys.exit()

img = Image.open(_filename).convert("1") #convert 1-bit mode
pixels = img.load()
width = img.size[0]
height = img.size[1]

print "Image is %dx%d pixels" % img.size
if height != 32:
    print "Image MUST be 32 pixels high!"
    sys.exit()
if width > 128:
    print "Image cannot be more than 128 pixels wide!"
    sys.exit()

if _invert:
    blackVal = 255
else:
    blackVal = 0

for x in range(width):
    col = 0L
    for y in range(height):
        val = pixels[x, y]
        bit = y
        if _flip_v:
            bit = 31 - y
        if val == blackVal:
            col += (1 << bit)
    cols.append(col)

if _flip_h:
    cols = list(reversed(cols))

if _header != "":
    header = genHeader(cols, _delay)
    print "Writing header to %s" % _header
    file = open(_header, "w")
    file.write(header)
    file.close()

printCols(cols)

print "Done!"