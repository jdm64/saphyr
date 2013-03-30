/*      Saphyr, a C++ style compiler using LLVM
        Copyright (C) 2012, Justin Madru (justin.jdm64@gmail.com)

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __Constants__
#define __Constants__

enum class NodeType
{
	BaseNodeList, BaseType, VarDecl, Variable, Parameter, VariableDecGroup, FunctionDec,
	ReturnStm, Assignment, CompareOp, BinaryMathOp, FunctionCall, IntConst, FloatConst,
	LogicalOp, WhileStm, LoopBranch, ForStm, TernaryOp, Increment, UnaryMath, IfStm,
	NullCoalescing, FunctionProto, Label, Goto, GlobalVarDecl
};

enum class BaseDataType
{
	AUTO, VOID, BOOL, INT, INT8, INT16, INT32, INT64, FLOAT, DOUBLE
};

#endif
