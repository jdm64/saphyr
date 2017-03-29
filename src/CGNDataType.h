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
#ifndef __CG_NDATATYPE_H__
#define __CG_NDATATYPE_H__

class CGNDataType
{
protected:
	typedef SType* (CGNDataType::*classPtr)(NDataType*);

	CodeContext& context;

	explicit CGNDataType(CodeContext& context)
	: context(context) {}

	virtual ~CGNDataType() {}

	SType* visitNBaseType(NBaseType* type);

	SType* visitNConstType(NConstType* type);

	SType* visitNThisType(NThisType* type);

	SType* visitNArrayType(NArrayType* type);

	SType* visitNVecType(NVecType* type);

	SType* visitNUserType(NUserType* type);

	SType* visitNPointerType(NPointerType* type);

	SType* visitNFuncPointerType(NFuncPointerType* type);

	virtual SType* visit(NDataType* type);

	SType* getArrayType(NArrayType* type);

private:
	static classPtr* buildVTable();

	static classPtr *vtable;

public:

	static SType* run(CodeContext& context, NDataType* type)
	{
		CGNDataType runner(context);
		return runner.visit(type);
	}
};

class CGNDataTypeNew : public CGNDataType
{
	static classPtr *vtable;

	RValue sizeVal;

	explicit CGNDataTypeNew(CodeContext& context)
	: CGNDataType(context) {}

	SType* visitNBaseType(NBaseType* type);

	SType* visitNConstType(NConstType* type);

	SType* visitNThisType(NThisType* type);

	SType* visitNArrayType(NArrayType* type);

	SType* visitNVecType(NVecType* type);

	SType* visitNUserType(NUserType* type);

	SType* visitNPointerType(NPointerType* type);

	SType* visitNFuncPointerType(NFuncPointerType* type);

	SType* visit(NDataType* type);

	static classPtr* buildVTable();

	void setSize(SType* ty);

	void setSize(uint64_t size);

	void setSize(const RValue& size);

public:

	static SType* run(CodeContext& context, NDataType* type, RValue& size);
};

#endif
