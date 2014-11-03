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

class TestCase:
	def __init__(self, file, clean=True, update=False):
		self.tstFile = file
		self.doClean = clean
		self.doUpdate = update
		self.basename = file[0 : file.rfind(".")]
		self.srcFile = self.basename + SYP_EXT
		self.expFile = self.basename + EXP_EXT
		self.llFile = self.basename + LL_EXT
		self.errFile = self.basename + ERR_EXT
		self.negFile = self.basename + NEG_EXT

	def createFiles(self):
		with open(self.tstFile) as testFile:
			data = testFile.read().split("========")
			if len(data) != 2:
				return True
			with open(self.srcFile, "w") as sourceFile:
				sourceFile.write(data[0])
			with open(self.expFile, "w") as asmFile:
				asmFile.write(data[1].lstrip())
		return False

	def update(self, isPos):
		expected = self.llFile if isPos else self.negFile
		with open(self.srcFile) as sourceF, open(expected) as expF, open(self.tstFile, "w") as tstF:
			tstF.write("\n" + sourceF.read().strip() + "\n\n")
			tstF.write("========\n\n")
			tstF.write(expF.read())

	def clean(self):
		ext_list = [SYP_EXT, LL_EXT, EXP_EXT, ERR_EXT, NEG_EXT]
		Cmd(["rm"] + [self.basename + ext for ext in ext_list])

	def patchAsm(self, file):
		with open(file, "r") as asm:
			data = asm.read()
		data = re.sub("; ModuleID =.*", "", data).strip() + "\n"
		with open(file, "w") as asm:
			asm.write(data)

	def writeLog(self, p):
		with open(self.errFile, "w") as log:
			log.write(p.err)
			log.write(p.out)

	def runExe(self):
		proc = Cmd([SAPHYR_BIN, self.srcFile])
		if proc.ext < 0:
			self.writeLog(proc)
			return True, "[crash]"
		elif proc.ext == 0:
			actual = self.llFile
			self.patchAsm(actual)
			isPos = True
		else:
			actual = self.negFile
			with open(actual, "w") as err:
				err.write(proc.err + proc.out)
			isPos = False

		proc = Cmd(["diff", "-uwB", self.expFile, actual])
		if proc.ext == 0:
			return False, "[ok]"
		elif self.doUpdate:
			self.update(isPos)
			return False, "[updated]"
		else:
			self.writeLog(proc)
			return True, "[output differs]"

	def run(self):
		if self.createFiles():
			return True, "[missing section]"
		res = self.runExe()
		if not res[0] and self.doClean:
			self.clean()
		return res

def cleanTests(files):
	files = getFiles(files)
	if not files:
		print("No tests found")
		return 1
	for file in files:
		TestCase(file).clean()
	return 0

def runTests(files, clean=True, update=False):
	files = getFiles(files)
	if not files:
		print("No tests found")
		return 1
	padding = len(max(files, key=len))
	failed = 0
	total = len(files)
	for file in files:
		error, msg = TestCase(file, clean, update).run()
		print(file.ljust(padding) + " = " + msg)
		failed += error
	passed = total - failed
	print(str(passed) + " / " + str(total) + " tests passed")
	return 1 if failed else 0

def main():
	args = sys.argv[1:]
	if not args:
		return runTests(args)

	return {
	"--clean": lambda: cleanTests(args[1:]),
	      "-c": lambda: cleanTests(args[1:]),
	"--update": lambda: runTests(args[1:], True, True),
	      "-u": lambda: runTests(args[1:], True, True),
	  "--dump": lambda: runTests(args[1:], False, False),
	      "-d": lambda: runTests(args[1:], False, False)
	}.get(args[0], lambda: runTests(args))()

if __name__ == "__main__":
	sys.exit(main())
