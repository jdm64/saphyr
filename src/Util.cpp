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

#include "Util.h"

/**
 * From: boost::filesystem src/operations.cpp
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */
path Util::relative(const path& p)
{
	return Util::lexically_relative(Util::weakly_canonical(p), Util::weakly_canonical(current_path()));
}

/**
 * From: boost::filesystem src/path.cpp
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */
inline std::pair<path::iterator, path::iterator> mismatch(path::iterator it1, path::iterator it1end, path::iterator it2, path::iterator it2end)
{
	for (; it1 != it1end && it2 != it2end && *it1 == *it2;) {
		++it1;
		++it2;
	}
	return std::make_pair(it1, it2);
}

/**
 * From: boost::filesystem src/path.cpp
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */
path Util::lexically_relative(const path& p, const path& base)
{
	std::pair<path::iterator, path::iterator> mm = mismatch(p.begin(), p.end(), base.begin(), base.end());
	if (mm.first == p.begin() && mm.second == base.begin())
		return path();
	if (mm.first == p.end() && mm.second == base.end())
		return ".";
	path tmp;
	for (; mm.second != base.end(); ++mm.second)
		tmp /= "..";
	for (; mm.first != p.end(); ++mm.first)
		tmp /= *mm.first;
	return tmp;
}

/**
 * From: boost::filesystem src/path.cpp
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */
path Util::lexically_normal(const path& p)
{
	if (p.empty())
		return p;

	path temp;
	auto start = p.begin();
	auto last = p.end();
	auto stop = last--;

	for (auto itr(start); itr != stop; ++itr) {
		// ignore "." except at start and last
		if (itr->native().size() == 1 && (itr->native())[0] == '.' && itr != start && itr != last)
			continue;

		// ignore a name and following ".."
		if (!temp.empty() && itr->native().size() == 2 && (itr->native())[0] == '.' && (itr->native())[1] == '.') { // dot dot
			auto lf = temp.filename().native();
			if (lf.size() > 0 && (lf.size() != 1 || (lf[0] != '.' && lf[0] != '/')) && (lf.size() != 2  || (lf[0] != '.' && lf[1] != '.'))) {
				temp.remove_filename();

				auto next = itr;
				if (temp.empty() && ++next != stop && next == last && *last == ".") {
					temp /= ".";
				}
				continue;
			}
		}
		temp /= *itr;
	}
	if (temp.empty())
		temp /= ".";
	return temp;
}

/**
 * From: boost::filesystem src/operations.cpp
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */
path Util::weakly_canonical(const path& p)
{
	path head(p);
	path tail;
	path::iterator itr = p.end();

	for (; !head.empty(); --itr) {
		file_status head_status = status(head);
		if (head_status.type() != file_not_found)
			break;
		head.remove_filename();
	}

	bool tail_has_dots = false;
	for (; itr != p.end(); ++itr) {
		tail /= *itr;
		// for a later optimization, track if any dot or dot-dot elements are present
		if (itr->native().size() <= 2 && itr->native()[0] == '.' && (itr->native().size() == 1 || itr->native()[1] == '.'))
			tail_has_dots = true;
	}

	if (head.empty())
		return Util::lexically_normal(p);
	head = canonical(head);
	return tail.empty()
		? head
		: (tail_has_dots  // optimization: only normalize if tail had dot or dot-dot element
		? Util::lexically_normal(head / tail)
		: head/tail);
}
