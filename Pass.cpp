/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
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
#include <llvm/Instructions.h>
#include <llvm/Support/CFG.h>
#include "Pass.h"

using namespace std;

char SimpleBlockClean::ID = 0;

bool SimpleBlockClean::removeBranchBlock(BasicBlock* block)
{
	auto& first = block->front();
	if (!isa<BranchInst>(first))
		return false;

	auto branch = static_cast<BranchInst*>(&first);
	if (branch->isConditional())
		return false;

	auto branchTo = branch->getSuccessor(0);
	vector<TerminatorInst*> termVec;
	vector<BasicBlock*> predBlocks;
	for (auto pred = pred_begin(block), end = pred_end(block); pred != end; ++pred) {
		auto blk = *pred;
		predBlocks.push_back(blk);
		termVec.push_back(blk->getTerminator());
	}
	for (auto& inst : termVec) {
		for (int i = 0; i < inst->getNumSuccessors(); i++) {
			auto successor = inst->getSuccessor(i);
			if (successor == block)
				inst->setSuccessor(i, branchTo);
		}
	}
	// removing a branch will invalidate any PHI instructions
	// BUG: will a PHI always be first?
	auto& inst = branchTo->front();
	if (isa<PHINode>(inst)) {
		auto phi = static_cast<PHINode*>(&inst);
		auto val = phi->removeIncomingValue(block, false);
		for (auto pBlock : predBlocks)
			phi->addIncoming(val, pBlock);
	}
	return true;
}

bool SimpleBlockClean::runOnFunction(Function &func)
{
	bool modified = false;

	auto end = func.end();
	auto iter = func.begin();
	iter++; // skip first block
	while (iter != end) {
		if (iter->empty() || pred_begin(&*iter) == pred_end(&*iter)) {
			(iter++)->eraseFromParent();
			modified = true;
		} else if (iter->size() == 1) {
			if (removeBranchBlock(&*iter)) {
				(iter++)->eraseFromParent();
				modified = true;
			} else {
				++iter;
			}
		} else {
			++iter;
		}
	}
	return modified;
}
