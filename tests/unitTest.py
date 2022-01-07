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

import os, sys, fnmatch, codecs, difflib
from subprocess import Popen, PIPE

SAPHYR_BIN = "../saphyr"
SYFMT_BIN = "../syfmt"
ENCODING = "utf-8"
TEST_EXT = ".test"
SYP_EXT = ".syp"
FMT_EXT = ".syp.txt"
LL_EXT = ".ll"
ERR_EXT = ".err"
EXP_EXT = ".exp"
NEG_EXT = ".neg"
BC_EXT = ".bc"
OBJ_EXT = ".o"

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
	for root, _, filenames in os.walk('.'):
		root = root.strip("./")
		for filename in fnmatch.filter(filenames, '*' + TEST_EXT):
			matches.append(os.path.join(root, filename))
	return sorted(matches)

def getFiles(files):
	allTests = findAllTests()
	if not files:
		return allTests

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

def patchAsm(filename):
	data = ""
	with open(filename, "r") as asm:
		for line in asm:
			if not "; ModuleID" in line and not "source_filename" in line:
				data += line
	data = data.strip() + "\n"
	with open(filename, "w") as asm:
		asm.write(data)

class TestCase:
	def __init__(self, file, fromVer=None, toVer=None):
		self.tstFile = file
		self.fromVer = fromVer
		if self.fromVer != None and len(self.fromVer) > 0:
			self.fromVer = "-" + self.fromVer
		self.toVer = toVer
		if self.toVer != None and len(self.toVer) > 0:
			self.toVer = "-" + self.toVer
		self.basename = file[0 : file.rfind(".")]
		self.srcFile = self.basename + SYP_EXT
		self.expFile = self.basename + EXP_EXT
		self.llFile = self.basename + LL_EXT
		self.objFile = self.basename + OBJ_EXT
		self.errFile = self.basename + ERR_EXT
		self.negFile = self.basename + NEG_EXT

	def createFiles(self):
		with codecs.open(self.tstFile, "r", ENCODING) as testFile:
			data = testFile.read().split("========")
			if len(data) < 2:
				return True, "[missing section]"

			with codecs.open(self.srcFile, "w", ENCODING) as sourceFile:
				sourceFile.write(data[0])
			with codecs.open(self.expFile, "w", ENCODING) as asmFile:
				asmFile.write(data[1].lstrip())

			self.expSyms = data[2].lstrip() if len(data) >= 3 else "\n"
			self.actSyms = None

		return False, "[dump]"

	def update(self, isPos):
		expected = self.llFile if isPos else self.negFile
		with codecs.open(self.srcFile, "r", ENCODING) as sourceF, codecs.open(expected, "r", ENCODING) as expF, codecs.open(self.tstFile, "w", ENCODING) as tstF:
			tstF.write("\n" + sourceF.read().strip() + "\n\n")
			tstF.write("========\n\n")
			tstF.write(expF.read())
			if isPos:
				tstF.write("\n========\n\n")
				tstF.write(self.actSyms if self.actSyms else self.expSyms)

	def clean(self):
		ext_list = [SYP_EXT, FMT_EXT, LL_EXT, EXP_EXT, ERR_EXT, NEG_EXT, BC_EXT, OBJ_EXT]
		Cmd(["rm"] + [self.basename + ext for ext in ext_list])

	def rewriteIR(self, fileName):
		bcFile = self.basename + ".bc"
		proc = Cmd(["llvm-as" + self.fromVer, "-o", bcFile, fileName])
		if proc.ext != 0:
			return True, "[llvm-as error]"
		proc = Cmd(["llvm-dis" + self.toVer, "-o", fileName, bcFile])
		if proc.ext != 0:
			return True, "[llvm-dis error]"
		return False, None

	def fixIR(self):
		if not os.path.exists(self.llFile):
			return True, "[no .ll file]"
		if self.fromVer != None and self.toVer != None:
			err, msg = self.rewriteIR(self.expFile)
			if err:
				return err, msg
			patchAsm(self.expFile)
			err, msg = self.rewriteIR(self.llFile)
			if err:
				return err, msg
		patchAsm(self.llFile)
		return False, None

	def writeLog(self, p):
		with open(self.errFile, "w") as log:
			log.write(p.err)
			log.write(p.out)

	def getHeader(self):
		with codecs.open(self.srcFile, "r", ENCODING) as file:
			line = file.readline() + file.readline()
		return line

	def runFmt(self):
		header = self.getHeader()
		if header.find("nofmt") != -1 or header.find("print-debug") != -1:
			return False, None

		proc = Cmd([SYFMT_BIN, self.srcFile])
		if proc.ext < 0:
			self.writeLog(proc)
			return True, "[crash format]"
		elif proc.ext > 0:
			return False, None

		fmtFile = self.srcFile + ".txt"
		with codecs.open(fmtFile, "w", ENCODING) as file:
			file.write(proc.out)
		proc = Cmd(["diff", "-uwB", self.srcFile, fmtFile])
		if proc.ext != 0:
			self.writeLog(proc)
			return True, "[fail format]"
		return False, None

	def runExe(self, doUpdate):
		ret = self.runFmt()
		if ret[0]:
			return ret

		cmdline = [SAPHYR_BIN]
		if self.getHeader().find("print-debug") != -1:
			cmdline.append("--print-debug")
		cmdline.extend(["--llvmir", self.srcFile])

		proc = Cmd(cmdline)
		if proc.ext < 0:
			self.writeLog(proc)
			return True, "[crash compile]"
		elif proc.ext == 0:
			err, msg = self.fixIR()
			if err:
				return True, msg
			actual = self.llFile
			isPos = True
		else:
			actual = self.negFile
			with open(actual, "w") as err:
				err.write(proc.err + proc.out)
			isPos = False

		proc = Cmd(["diff", "-uwB", self.expFile, actual])
		if proc.ext == 0:
			if not isPos:
				return False, "[ok]"
		elif doUpdate:
			self.update(isPos)
			return False, "[updated compile]"
		else:
			self.writeLog(proc)
			return True, "[fail compile]"

		return self.runSym(doUpdate)

	def runSym(self, doUpdate):
		proc = Cmd(["nm", "-fp", self.objFile])
		if proc.ext != 0:
			self.writeLog(proc)
			return True, "[no symbols]"

		expected = self.expSyms.splitlines(1)
		actual = [" ".join(x.split(None, 2)[0:2]) + "\n" for x in proc.out.splitlines(1) if " r " not in x]
		diff = difflib.unified_diff(expected, actual, fromfile='expected.sym', tofile='actual.sym')
		diff = ''.join(diff)

		if not len(diff):
			return False, "[ok]"
		elif doUpdate:
			self.actSyms = "".join(actual)
			self.update(True)
			return False, "[updated symbols]"

		with open(self.errFile, "w") as log:
			log.write(diff)
		return True, "[diff symbols]"

	def run(self, doUpdate):
		err, msg = self.createFiles()
		if err:
			return True, msg

		err, msg = self.runExe(doUpdate)
		if not err:
			self.clean()
		return err, msg

def cleanTests(files):
	for file in files:
		TestCase(file).clean()
	return 0

def dumpFiles(files):
	padding = len(max(files, key=len))
	failed = 0
	total = len(files)
	for file in files:
		error, msg = TestCase(file).createFiles()
		print(file.ljust(padding) + " = " + msg)
		failed += error
	passed = total - failed
	print(str(passed) + " / " + str(total) + " tests dumped")
	return 1 if failed else 0

def runTests(files, doUpdate=False, fromVer=None, toVer=None):
	padding = len(max(files, key=len))
	failed = 0
	total = len(files)
	for file in files:
		error, msg = TestCase(file, fromVer, toVer).run(doUpdate)
		print(file.ljust(padding) + " = " + msg)
		failed += error
	passed = total - failed
	print(str(passed) + " / " + str(total) + " tests passed")
	return 1 if failed else 0

def main():
	args = sys.argv[1:]
	if not args:
		files = getFiles(None)
		return runTests(files)

	if args[0].startswith("-"):
		cmd = args[0]
		files = args[1:]
	else:
		cmd = "-r"
		files = args

	# if the first "file" starts with a '+' then we want to upgrade the
	# expected llvmIR. Example: +3.6-3.7
	if len(files) > 0 and files[0][0] == "+":
		fromVer, toVer = files[0][1:].split("-")
		files = files[1:]
	else:
		fromVer, toVer = None, None

	files = getFiles(files)
	if not files:
		print("No tests found")
		return 1

	return {
	"--clean": lambda: cleanTests(files),
	      "-c": lambda: cleanTests(files),
	"--update": lambda: runTests(files, True, fromVer, toVer),
	      "-u": lambda: runTests(files, True, fromVer, toVer),
	  "--dump": lambda: dumpFiles(files),
	      "-d": lambda: dumpFiles(files)
	}.get(cmd, lambda: runTests(files, False, fromVer, toVer))()

if __name__ == "__main__":
	sys.exit(main())
