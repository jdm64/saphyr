/* Saphyr, a C style compiler using LLVM
 * Copyright (C) 2009-2015, Justin Madru (justin.jdm64@gmail.com)
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

#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <string>

using namespace std;

class Token
{
public:
	Token()
	: line(0) {}

	Token(string token, string filename = "", int lineNum = 0)
	: str(std::move(token)), filename(std::move(filename)), line(lineNum) {}

	string str;
	string filename;
	int line;
};

#endif
