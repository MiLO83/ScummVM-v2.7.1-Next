/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 *
 */

#include "bladerunner/chapters.h"

#include "bladerunner/bladerunner.h"
#include "bladerunner/slice_animations.h"

namespace BladeRunner {

bool Chapters::enterChapter(int chapter) {
	// Chapter valid values are: 1 - 5 corresponding to the game Acts.
	// Before set, _chapter is 0 (see chapters.h -- The mapped resource id is still 1)
	// Resource id is (see _resourceIds in chapters.h):
	// 1 for chapter 0, 1 - Act 1
	// 2 for chapter 2, 3 - Act 2, 3
	// 3 for chapter 4    - Act 4  (see note and code below, 3 can be used for Act 5 too)
	// 4 for chapter 5    - Act 5
	// Note: For resources that only go up to "3" (TLK, VQA), 3 is used for both Acts 4 and 5.

	int id = _resourceIds[chapter];

	if (!_vm->_sliceAnimations->openFrames(id))
		return false;

	if (!_vm->openArchive("A.TLK"))
		return false;

	if (!_vm->openArchive(Common::String::format("VQA%d.MIX", MIN(id, 3))))
		return false;

	if (_vm->_cutContent) {
		for (int chi = 1; chi < 4; ++chi) {
			if (!_vm->isArchiveOpen(Common::String::format("%d.TLK", chi))
			    && !_vm->openArchive(Common::String::format("%d.TLK", chi))
			) {
				return false;
			}
		}
	} else {
		if (!_vm->openArchive(Common::String::format("%d.TLK", MIN(id, 3))))
			return false;
	}

	if (!_vm->openArchive(Common::String::format("OUTTAKE%d.MIX", id)))
		return false;

	_chapter = chapter;
	_hasOpenResources = true;
	return true;
}

void Chapters::closeResources() {
	int id = _resourceIds[_chapter];

#if BLADERUNNER_ORIGINAL_BUGS
	_vm->closeArchive("A.TLK");
#else
	if (_vm->isArchiveOpen("A.TLK")) {
		_vm->closeArchive("A.TLK");
	}
#endif // BLADERUNNER_ORIGINAL_BUGS
	_vm->closeArchive(Common::String::format("VQA%d.MIX", MIN(id, 3)));
	// It's better to try and close every TLK file here (if open), since
	// when switching from Restored Content version to Original (due to a save game load)
	// TLK files would still remain open -- and should still be closed here
	for (int chi = 1; chi < 4; ++chi) {
		if (_vm->isArchiveOpen(Common::String::format("%d.TLK", chi))) {
			_vm->closeArchive(Common::String::format("%d.TLK", chi));
		}
	}
	_vm->closeArchive(Common::String::format("OUTTAKE%d.MIX", id));
	_hasOpenResources = false;
}

} // End of namespace BladeRunner
