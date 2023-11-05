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

#ifndef ULTIMA_ULTIMA1_U1DIALOGS_GROCERY_H
#define ULTIMA_ULTIMA1_U1DIALOGS_GROCERY_H

#include "ultima/ultima1/u1dialogs/buy_sell_dialog.h"

namespace Ultima {
namespace Ultima1 {
namespace U1Dialogs {

using Shared::CFrameMsg;
using Shared::CTextInputMsg;

/**
 * Implements the buy/sell dialog for grocers
 */
class Grocery : public BuySellDialog {
	DECLARE_MESSAGE_MAP;
	bool TextInputMsg(CTextInputMsg *msg);
private:
	uint _costPerPack;
protected:
	/**
	 * Set the mode
	 */
	void setMode(BuySell mode) override;
public:
	CLASSDEF;

	/**
	 * Constructor
	 */
	Grocery(Ultima1Game *game, int groceryNum);

	/**
	 * Draws the visual item on the screen
	 */
	void draw() override;
};

} // End of namespace U1Dialogs
} // End of namespace Ultima1
} // End of namespace Ultima

#endif
