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

#include "ultima/ultima1/widgets/merchant_transport.h"
#include "ultima/ultima1/u1dialogs/transports.h"

namespace Ultima {
namespace Ultima1 {
namespace Widgets {

EMPTY_MESSAGE_MAP(MerchantTransport, Merchant);

void MerchantTransport::talk() {
	U1Dialogs::Transports *dialog = new U1Dialogs::Transports(_game, _map->getMapStyle() - 2);
	dialog->show();
}

} // End of namespace Widgets
} // End of namespace Ultima1
} // End of namespace Ultima
