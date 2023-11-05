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

#ifndef GLK_ALAN3_CONTAINER
#define GLK_ALAN3_CONTAINER

#include "glk/alan3/types.h"
#include "glk/alan3/acode.h"
#include "glk/jumps.h"

namespace Glk {
namespace Alan3 {

/* DATA */
extern ContainerEntry *containers; /* Container table pointer */


/* FUNCTIONS */
extern int containerSize(int container, ATrans trans);
extern bool passesContainerLimits(CONTEXT, Aint container, Aint addedInstance);
extern void describeContainer(CONTEXT, int container);
extern void list(CONTEXT, int cnt);

} // End of namespace Alan3
} // End of namespace Glk

#endif
