#must install Pillow (replacement for PIL) http://python-imaging.github.io/
#pip install Pillow
#Need to have VS2008 or higher installed on windows for this to work
#if higher than 2008, run one of the following commands at the console
#VS2010:
#SET VS90COMNTOOLS=%VS100COMNTOOLS%
#VS2012
#SET VS90COMNTOOLS=%VS110COMNTOOLS%
#VS2013
#SET VS90COMNTOOLS=%VS120COMNTOOLS%
from PIL import Image
import sys, getopt

def genHeader(cols, delay):
	output = ""
	output += "PROGMEM uint32_t imageData[] = {\n"
	for c in cols:
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
_filename = ""
_header = ""
_delay = 1

try:
	opts, args = getopt.getopt(sys.argv[1:], "f:o:p:b:", ["list","flip","invert",])
except getopt.GetoptError:
	print "EpochPOVGen.py help goes here"
	sys.exit()
for opt, arg in opts:
	if opt == '-f':
		_filename = arg
	elif opt == '-o':
		_header = arg;
	elif opt == '--flip':
		_flip = True
	elif opt == '--invert':
		_invert = True
	elif opt == '-p':
		print "Port specified: " + arg
		port = arg
	elif opt == '-b':
		try:
			baud = int(arg)
		except ValueError:
			print "Invalid baud rate specified. Must be a integer."
			sys.exit()
	elif opt == '--list':
		ports = [port[0] for port in list_ports.comports()]
		if len(ports) == 0:
			print "No available serial ports found!"
			sys.exit()
		print "Available serial ports:"
		for p in ports:
			print p
		sys.exit()

if _filename == "":
	print "Please use -f <file> to specify a file."
	sys.exit();

print "Loading Image: " + _filename
if _filename.endswith(".bmp") != True:
	print "File MUST be a .bmp file!"
	sys.exit()

img = Image.open(_filename).convert("1") #convert 1-bit mode
pixels = img.load();
width = img.size[0];
height = img.size[1];

print "Image is %dx%d pixels" % img.size
if height != 32:
	print "Image MUST be 32 pixels high!"
	sys.exit()
if width > 128:
	print "Image cannot be more than 128 pixels wide!"
	sys.exit()

if _invert:
	blackVal = 255;
else:
	blackVal = 0;

cols = []

for x in range(width):
	col = 0L
	for y in range(height):
		val = pixels[x, y]
		bit = y
		if _flip:
			bit = 32 - y
		if val == blackVal:
			col += (1 << bit)
	cols.append(col)

if _header != "":
	header = genHeader(cols, _delay)
	print "Writing header to %s" % _header
	file = open(_header, "w");
	file.write(header)
	file.close()

printCols(cols);

print "Done!"