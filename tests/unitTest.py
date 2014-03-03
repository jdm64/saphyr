#!/usr/bin/env python3
#
# Saphyr, a C++ style compiler using LLVM
# Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os, sys, fnmatch, re
from subprocess import call, Popen, PIPE

SAPHYR_BIN = "../saphyr"
ENCODING = "utf-8"
TEST_EXT = ".tst"
SYP_EXT = ".syp"
LL_EXT = ".ll"
ERR_EXT = ".err"
EXP_EXT = ".exp"
NEG_EXT = ".neg"
PADDING = 0

class Cmd:
	def __init__(self, cmd):
		p = Popen(cmd, stdout=PIPE, stderr=PIPE)
		p.wait()
		out, err = p.communicate()
		self.ext = p.returncode
		self.out = out.decode(ENCODING)
		self.err = err.decode(ENCODING)

def findAllTests():
	matches = []
	for root, dirnames, filenames in os.walk('.'):
		root = root.strip("./")
		for filename in fnmatch.filter(filenames, '*' + TEST_EXT):
			matches.append(os.path.join(root, filename))
	return matches

def getFiles(files):
	if not isinstance(files, type([])):
		files = [files]
	elif not files:
		return findAllTests()

	allTests = findAllTests()
	fileList = []
	for f in files:
		dot = f.rfind('.')
		if dot > 0:
			f = f[0:dot]
		matches = [item for item in allTests if f in item]
		fileList.extend(matches)
		for item in matches:
			allTests.remove(item)
	return fileList

def createTestFiles(filename):
	basename = filename[0 : filename.rfind(".")]
	with open(filename) as testFile:
		data = testFile.read().split("========")
		if len(data) != 2:
			return True
		with open(basename + SYP_EXT, "w") as sourceFile:
			sourceFile.write(data[0])
		with open(basename + EXP_EXT, "w") as asmFile:
			asmFile.write(data[1])

def updateTest(basename, isPos=True):
	expExt = LL_EXT if isPos else NEG_EXT
	with open(basename + SYP_EXT) as sourceF, open(basename + expExt) as expF, open(basename + TEST_EXT, "w") as tstF:
		tstF.write(sourceF.read())
		tstF.write("========\n\n")
		tstF.write(expF.read())

def cleanSingleTest(file):
	basename = file[0 : file.rfind(".")]
	Cmd(["rm", basename + SYP_EXT, basename + LL_EXT, basename + EXP_EXT, basename + ERR_EXT, basename + NEG_EXT])

def patchAsmFile(file):
	with open(file, "r") as asm:
		data = asm.read()

	data = re.sub("; ModuleID =.*", "", data).strip() + "\n"

	with open(file, "w") as asm:
		asm.write(data)

def writeLogFile(basename, p):
	with open(basename + ERR_EXT, "w") as log:
		log.write(p.err)
		log.write(p.out)

def checkNegativeTest(basename, update, p):
	file = basename + TEST_EXT
	with open(basename + NEG_EXT, "w") as err:
		err.write(p.out)
	p = Cmd(["diff", "-uwB", basename + EXP_EXT, basename + NEG_EXT])
	if p.ext != 0:
		if update:
			print(file.ljust(PADDING) + " = [updated]")
			updateTest(basename, False)
			return False
		print(file.ljust(PADDING) + " = [compile error]")
		writeLogFile(basename, p)
		return True
	print(file.ljust(PADDING) + " = [ok]")
	return False

def checkPositiveTest(basename, update):
	file = basename + TEST_EXT
	patchAsmFile(basename + LL_EXT)
	p = Cmd(["diff", "-uwB", basename + EXP_EXT, basename + LL_EXT])
	if p.ext != 0:
		if update:
			print(file.ljust(PADDING) + " = [updated]")
			updateTest(basename)
			return False
		print(file.ljust(PADDING) + " = [output differs]")
		writeLogFile(basename, p)
		return True
	print(file.ljust(PADDING) + " = [ok]")
	return False

def runSingleTest(file, update=False):
	basename = file[0 : file.rfind(".")]
	p = Cmd([SAPHYR_BIN, basename + SYP_EXT])
	if p.ext != 0:
		return checkNegativeTest(basename, update, p)
	return checkPositiveTest(basename, update)

def cleanTests(files):
	files = getFiles(files)
	for file in files:
		cleanSingleTest(file)

def runTests(files, clean=True, update=False):
	global PADDING
	files = getFiles(files)
	if not files:
		return
	PADDING = len(max(files, key=len))
	failed = 0
	total = len(files)
	for file in files:
		if createTestFiles(file):
			print(file.ljust(PADDING) + " = [missing section]")
			failed += 1
			continue
		error = runSingleTest(file, update)
		if not error and clean:
			cleanSingleTest(file)
		failed += error
	passed = total - failed
	print(str(passed) + " / " + str(total) + " tests passed")

def main():
	args = sys.argv[1:]
	if not args:
		runTests(args)
		return

	{"--clean": lambda: cleanTests(args[1:]),
	      "-c": lambda: cleanTests(args[1:]),
	"--update": lambda: runTests(args[1:], True, True),
	      "-u": lambda: runTests(args[1:], True, True),
	  "--dump": lambda: runTests(args[1:], False, False),
	      "-d": lambda: runTests(args[1:], False, False)
	}.get(args[0], lambda: runTests(args))()

if __name__ == "__main__":
	main()
