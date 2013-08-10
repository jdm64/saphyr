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
#include <llvm/Support/CFG.h>
#include "Pass.h"

char SimpleBlockClean::ID = 0;

bool SimpleBlockClean::runOnFunction(Function &func)
{
	bool modified = false;

	auto end = func.end();
	auto iter = func.begin();
	while (iter != end) {
		if (iter->empty() || (iter->size() == 1 && pred_begin(&*iter) == pred_end(&*iter))) {
			(iter++)->eraseFromParent();
			modified = true;
		} else {
			++iter;
		}
	}
	return modified;
}
