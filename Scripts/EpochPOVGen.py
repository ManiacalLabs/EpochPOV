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
import sys
import getopt
import struct
import os

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

system = os.name

if system != 'nt':
	if os.getuid() != 0:
		print "Please re-run with sudo"
		raw_input("Press enter to continue...")
		sys.exit()

try:
	import serial.tools
except ImportError, e:
	print "Attempting to download and install pyserial..."
			
	import urllib2
	url = "https://pypi.python.org/packages/source/p/pyserial/pyserial-2.6.tar.gz"
	file_name = url.split('/')[-1]
	u = urllib2.urlopen(url)
	f = open(file_name, 'wb')
	meta = u.info()
	file_size = int(meta.getheaders("Content-Length")[0])
	print("Downloading: {0} Bytes: {1}".format(url, file_size))

	file_size_dl = 0
	block_sz = 8192
	while True:
		buffer = u.read(block_sz)
		if not buffer:
			break

		file_size_dl += len(buffer)
		f.write(buffer)
		p = float(file_size_dl) / file_size
		status = r"{0}  [{1:.2%}]".format(file_size_dl, p)
		status = status + chr(8)*(len(status)+1)
		sys.stdout.write(status)

	f.close()
	
	try:
		with open(file_name): pass
	except IOError:
		print "There was an error downloading pyserial! Check your connection."
		raw_input("Press enter to continue...")
		sys.exit()
		
	print("Extracting pyserial...")
	import tarfile
	try:
		tar = tarfile.open(file_name)
		tar.extractall()
		tar.close()
	except:
		print "There was an error extracting pyserial!"
		raw_input("Press enter to continue...")
		sys.exit()
		
	print('Installing pyserial...')
	os.chdir('pyserial-2.6')
	os.system('python setup.py install')
	os.chdir('..')
	
	raw_input("Install Complete. Please re-run the script. Press enter to continue...")
	sys.exit()

from serial.tools import list_ports 

port = ''
baud = 115200

_invert = False
_flip = False
_upload = False
_filename = ""
_header = ""
_delay = 1
_clear = False

try:
	opts, args = getopt.getopt(sys.argv[1:], "f:o:p:b:", ["list-com","flip","invert","upload","clear"])
except getopt.GetoptError:
	print "EpochPOVGen.py help goes here"
	sys.exit()
for opt, arg in opts:
	if opt == '-f':
		_filename = arg
	elif opt == '-o':
		_header = arg
	elif opt == '--flip':
		_flip = True
	elif opt == '--invert':
		_invert = True
	elif opt == '-p':
		print "Port specified: " + arg
		port = arg
	elif opt == '--upload':
		_upload = True;
	elif opt == '--clear':
		_clear = True
		_upload = True
	elif opt == '-b':
		try:
			baud = int(arg)
		except ValueError:
			print "Invalid baud rate specified. Must be a integer."
			sys.exit()
	elif opt == '--list-com':
		ports = [port[0] for port in list_ports.comports()]
		if len(ports) == 0:
			print "No available serial ports found!"
			sys.exit()
		print "Available serial ports:"
		for p in ports:
			print p
		sys.exit()

cols = []

if _clear == False:
	if _filename == "":
		print "Please use -f <file> to specify a file."
		sys.exit()

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
			if _flip:
				bit = 32 - y
			if val == blackVal:
				col += (1 << bit)
		cols.append(col)

	if _header != "":
		header = genHeader(cols, _delay)
		print "Writing header to %s" % _header
		file = open(_header, "w")
		file.write(header)
		file.close()

#printCols(cols)
com = None
if _upload:
	print "Staring Upload"
	if port == '':
		ports = [port for port in list_ports.comports()]
	if len(ports) > 0:
		if system == 'posix':
			p = ports[len(ports) - 1]
			port = p[0]
		elif system == 'nt':
			p = ports[0]
			port = p[0]
		elif system == 'mac':
			p = ports[len(ports) - 1]
			port = p[0]
		else:
			print "What system are you running?!"
			sys.exit()
		print "No port specified, using best guess serial port:\r\n" + p[1] + ", " + p[2] + "\r\n"
	else:
		print "Cannot find default port and no port given!"
		sys.exit()

	try:
		com = serial.Serial(port, baud, timeout=1);
		print "Connected to " + port + " @ " + str(baud) + " baud"
	except serial.SerialException, e:
		print "Unable to connect to the given serial port!\r\nTry the --list option to list available ports."
		print e
		sys.exit()

	if _clear:
		_delay = 0
		cols = []
		data = ""
	else:
		#struct.pack("!%sI" % len(cols), *cols)
		data = struct.pack("!%sI" % len(cols), *cols)
		print cols
		#print data
	try:
		print "Sending image data..."
		b = com.write(bytes("d" + chr(_delay) + chr(len(cols)) + data))
		print "write bytes: " + str(b)
		res = com.read()
		print "result: " + str(ord(res))
		if(b > 0 and res == '*'):
			print "Success sending data!"
		else:
			print "There was an error sending the data! Make sure your EpochPOV is in Serial Data Mode"
		com.close()
	except serial.SerialTimeoutException:
		print "Timeout sending data! Please check your serial connection"
		sys.exit()


print "Done!"