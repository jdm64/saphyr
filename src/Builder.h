/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2016, Justin Madru (justin.jdm64@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BUILDER_H__
#define __BUILDER_H__

class Builder
{
	static SFunction lookupFunction(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params);

	static SFunctionType* getFuncType(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params);

	static bool addMembers(NVariableDeclGroup* group, vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context);

public:
	static SFunctionType* getFuncType(CodeContext& context, Token* name, NDataType* retType, NDataTypeList* params);

	static SFunction CreateFunction(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params, NStatementList* body);

	static void CreateClassFunction(CodeContext& context, Token* name, NClassDeclaration* theClass, NDataType* rtype, NParameterList* params, NStatementList* body);

	static void CreateStruct(CodeContext& context, NStructDeclaration::CreateType ctype, Token* name, NVariableDeclGroupList* list);
};

#endif
