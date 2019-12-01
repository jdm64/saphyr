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
#ifndef __FORMAT_CONTEXT_H__
#define __FORMAT_CONTEXT_H__

class FormatContext
{
	vector<string> lines;
	vector<string>* altLines = nullptr;
	int indentLvl = 0;

	string indentLine() const
	{
		string r;
		for (int i = 0; i < indentLvl; i++)
			r += "\t";
		return r;
	}

public:

	void addLine(const string& line)
	{
		(altLines ? *altLines : lines).push_back(indentLine() + line);
	}

	void add(const string& str)
	{
		(altLines ? *altLines : lines).back() += str;
	}

	void setBuffer(vector<string>* alt)
	{
		altLines = alt;
	}

	int getIndent() const
	{
		return indentLvl;
	}

	void setIndent(int indent)
	{
		indentLvl = indent;
	}

	void indent()
	{
		indentLvl++;
	}

	void undent()
	{
		indentLvl--;
	}

	void print()
	{
		string buff;
		for (string l : lines)
			buff += l + "\n";
		cout << buff;
	}
};

#endif
