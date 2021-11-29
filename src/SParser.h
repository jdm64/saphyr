/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2021, Justin Madru (justin.jdm64@gmail.com)
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


#ifndef __SPARSER_H__
#define __SPARSER_H__

#include "parser.h"
#include "BaseNodes.h"


class SParser : public Parser
{
	uPtr<Scanner> lexer;
	uPtr<NStatementList> root;

public:
	SParser(const string& filename)
	{
		lexer = uPtr<Scanner>(new Scanner(filename, "-"));
		lexer->setSval(getSval());
	}

	void setRoot(NStatementList* ptr)
	{
		root = uPtr<NStatementList>(ptr);
	}

	Token getError()
	{
		string token = lexer->matched().size()? lexer->matched() : "<EOF>";
		return Token("Syntax error on: " + token, lexer->filename(), lexer->lineNr(), lexer->colNr());
	}

	NStatementList* getRoot()
	{
		return root.get();
	}

	void setFilename(string name)
	{
		lexer->setFilename(name);
	}

	int lex()
	{
		return lexer->lex();
	}
};

#endif
