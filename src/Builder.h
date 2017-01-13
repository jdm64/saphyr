/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2017, Justin Madru (justin.jdm64@gmail.com)
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
	static SFunctionType* getFuncType(CodeContext& context, NDataType* rtype, NParameterList* params);

	static bool addMembers(NVariableDeclGroup* group, vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context);

	static bool isDeclared(CodeContext& context, Token* name);

	static void validateAttrList(CodeContext& context, NAttributeList* attrs);

public:
	static SFunctionType* getFuncType(CodeContext& context, NDataType* retType, NDataTypeList* params);

	static SFunction getFuncPrototype(CodeContext& context, Token* name, SFunctionType* funcType, NAttributeList* attrs = nullptr);

	static SFunction CreateFunction(CodeContext& context, Token* name, NDataType* rtype, NParameterList* params, NStatementList* body, NAttributeList* attrs = nullptr);

	static void CreateClassFunction(CodeContext& context, NClassFunctionDecl* stm, bool prototype);

	static void CreateClassConstructor(CodeContext& context, NClassConstructor* stm, bool prototype);

	static void CreateClassDestructor(CodeContext& context, NClassDestructor* stm, bool prototype);

	static void CreateClass(CodeContext& context, NClassDeclaration* stm, function<void(int)> visitor);

	static void CreateStruct(CodeContext& context, NStructDeclaration::CreateType ctype, Token* name, NVariableDeclGroupList* list);

	static void CreateEnum(CodeContext& context, NEnumDeclaration* stm);

	static void CreateAlias(CodeContext& context, NAliasDeclaration* stm);

	static void CreateOpaque(CodeContext& context, NOpaqueDecl* stm);

	static void CreateGlobalVar(CodeContext& context, NGlobalVariableDecl* stm, bool declaration);

	static void LoadImport(CodeContext& context, const string& filename);
};

#endif
