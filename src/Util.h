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

#include <boost/filesystem.hpp>

using namespace boost::filesystem;
using namespace std;

class Util
{
	/**
	 * From: boost::filesystem src/path.cpp
	 *
	 * Distributed under the Boost Software License, Version 1.0.
	 * See http://www.boost.org/LICENSE_1_0.txt
	 */
	static path lexically_relative(const path& p, const path& base);

	/**
	 * From: boost::filesystem src/path.cpp
	 *
	 * Distributed under the Boost Software License, Version 1.0.
	 * See http://www.boost.org/LICENSE_1_0.txt
	 */
	static path lexically_normal(const path& p);

	/**
	 * From: boost::filesystem src/operations.cpp
	 *
	 * Distributed under the Boost Software License, Version 1.0.
	 * See http://www.boost.org/LICENSE_1_0.txt
	 */
	static path weakly_canonical(const path& p);

public:
	/**
	 * From: boost::filesystem src/operations.cpp
	 *
	 * Distributed under the Boost Software License, Version 1.0.
	 * See http://www.boost.org/LICENSE_1_0.txt
	 */
	static path relative(const path& p);

	static string GetEnv(string name);

	static path getDataDir();

	static string getErrorFilename(const path& p);
};
