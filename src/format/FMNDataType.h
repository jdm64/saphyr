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
#ifndef __FM_NDATATYPE_H__
#define __FM_NDATATYPE_H__

class FMNDataType
{
private:
	FormatContext& context;

	explicit FMNDataType(FormatContext& context)
	: context(context) {}

	string visitNBaseType(NBaseType* type);

	string visitNConstType(NConstType* type);

	string visitNThisType(NThisType* type);

	string visitNArrayType(NArrayType* type);

	string visitNVecType(NVecType* type);

	string visitNUserType(NUserType* type);

	string visitNPointerType(NPointerType* type);

	string visitNReferenceType(NReferenceType* type);

	string visitNFuncPointerType(NFuncPointerType* type);

	string visit(NDataType* type);

	string getArrayType(NArrayType* type);

public:

	static string run(FormatContext& context, NDataType* type)
	{
		FMNDataType runner(context);
		return runner.visit(type);
	}
};

#endif
