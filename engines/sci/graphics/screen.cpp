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

#include "common/config-manager.h"
#include "common/util.h"
#include "common/system.h"
#include "common/timer.h"
#include "graphics/surface.h"
#include "graphics/palette.h"
#include "graphics/cursorman.h"
#include "engines/util.h"

#include "sci/sci.h"
#include "sci/engine/state.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/view.h"
#include "sci/graphics/palette.h"
#include "sci/graphics/scifx.h"
#include "sci/graphics/picture.h"

namespace Sci {

extern bool playingVideoCutscenes;
extern float blackFade;
GfxScreen::GfxScreen(ResourceManager *resMan) : _resMan(resMan) {

	// Scale the screen, if needed
	_upscaledHires = GFX_SCREEN_UPSCALED_320x200_X_EGA;
	if (_resMan->getViewType() != kViewEga) {
		_upscaledHires = GFX_SCREEN_UPSCALED_320x200_X_VGA;
	}
	// we default to scripts running at 320x200
	_scriptWidth = 320;
	_scriptHeight = 200;
	_width = 0;
	_height = 0;
	_displayWidth = 0;
	_displayHeight = 0;

	_curPaletteMapValue = 0;
	_paletteModsEnabled = false;

	// King's Quest 6 has hires content in the Windows version which we also
	// allow to be optionally enabled in the DOS version.
	if (g_sci->getGameId() == GID_KQ6) {
		if ((g_sci->getPlatform() == Common::kPlatformWindows) ||
			(g_sci->getPlatform() == Common::kPlatformDOS && g_sci->forceHiresGraphics())) {
			_upscaledHires = GFX_SCREEN_UPSCALED_640x440;
		}
	}

	// Korean versions of games use hi-res font on upscaled version of the game.
	if ((g_sci->getLanguage() == Common::KO_KOR) && (getSciVersion() <= SCI_VERSION_1_1))
		_upscaledHires = GFX_SCREEN_UPSCALED_640x400;
	// Japanese versions of games use hi-res font on upscaled version of the game.
	if ((g_sci->getLanguage() == Common::JA_JPN) && (getSciVersion() <= SCI_VERSION_1_1))
		_upscaledHires = GFX_SCREEN_UPSCALED_640x400;

	if (g_sci->getPlatform() == Common::kPlatformMacintosh) {
		if (getSciVersion() <= SCI_VERSION_01) {
			// Macintosh SCI0 games used 480x300, while the scripts were running at 320x200
			_upscaledHires = GFX_SCREEN_UPSCALED_480x300;
			_width = 480;
			_height = 300; // regular visual, priority and control map are 480x300 (this is different than other upscaled SCI games)
		} else {
			// Macintosh SCI1/1.1 games use hi-res native fonts if hi-res graphics are enabled
			if (g_sci->hasMacFonts() && g_sci->forceHiresGraphics()) {
				_upscaledHires = GFX_SCREEN_UPSCALED_640x400;
			}
		}

		// Some Mac SCI1/1.1 games only take up 190 rows and do not
		// have the menu bar.
		// TODO: Verify that LSL1 and LSL5 use height 190
		switch (g_sci->getGameId()) {
		case GID_FREDDYPHARKAS:
		case GID_KQ5:
		case GID_KQ6:
		case GID_LSL1:
		case GID_LSL5:
		case GID_SQ1:
			_scriptHeight = 190;
			break;
		default:
			break;
		}
	}

	// if not yet set, set those to script-width/height
	if (!_width)
		_width = _scriptWidth;
	if (!_height)
		_height = _scriptHeight;

	_pixels = _width * _height;

	switch (_upscaledHires) {
	case GFX_SCREEN_UPSCALED_480x300:
		// Space Quest 3, Hoyle 1+2 on MAC use this one
		_displayWidth = 480 * g_sci->_enhancementMultiplier;
		_displayHeight = 300 * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptHeight; i++)
			_upscaledHeightMapping[i] = ((i * 3) >> 1) * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptWidth; i++)
			_upscaledWidthMapping[i] = ((i * 3) >> 1) * g_sci->_enhancementMultiplier;
		break;
	case GFX_SCREEN_UPSCALED_640x400:
		// Police Quest 2 and Quest For Glory on PC9801 (Japanese)
		_displayWidth = 640 * g_sci->_enhancementMultiplier;
		_displayHeight = 400 * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptHeight; i++)
			_upscaledHeightMapping[i] = i * 2 * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptWidth; i++)
			_upscaledWidthMapping[i] = i * 2 * g_sci->_enhancementMultiplier;
		break;
	case GFX_SCREEN_UPSCALED_640x440:
		// used by King's Quest 6 on Windows
		_displayWidth = 640 * g_sci->_enhancementMultiplier;
		_displayHeight = 440 * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptHeight; i++)
			_upscaledHeightMapping[i] = ((i * 11) / 5) * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptWidth; i++)
			_upscaledWidthMapping[i] = (i * 2) * g_sci->_enhancementMultiplier;
		break;
	case GFX_SCREEN_UPSCALED_640x480:
		// Gabriel Knight 1 (VESA, Mac)
		_displayWidth = 640 * g_sci->_enhancementMultiplier;
		_displayHeight = 480 * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptHeight; i++)
			_upscaledHeightMapping[i] = ((i * 12) / 5) * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptWidth; i++)
			_upscaledWidthMapping[i] = (i * 2) * g_sci->_enhancementMultiplier;
		break;
	case GFX_SCREEN_UPSCALED_320x200_X_VGA:
		// 320x200 Games Enhanced with ScummVMX
		_displayWidth = _width * g_sci->_enhancementMultiplier;
		_displayHeight = _height * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptHeight; i++)
			_upscaledHeightMapping[i] = i * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptWidth; i++)
			_upscaledWidthMapping[i] = i * g_sci->_enhancementMultiplier;
		break;
	case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		// 320x200 Games Enhanced with ScummVMX
		_displayWidth = _width * g_sci->_enhancementMultiplier;
		_displayHeight = _height * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptHeight; i++)
			_upscaledHeightMapping[i] = i * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptWidth; i++)
			_upscaledWidthMapping[i] = i * g_sci->_enhancementMultiplier;
		break;
	default:
			_displayWidth = _width * g_sci->_enhancementMultiplier;
			_displayHeight = _height * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptHeight; i++)
			_upscaledHeightMapping[i] = i * g_sci->_enhancementMultiplier;
		for (int i = 0; i <= _scriptWidth; i++)
			_upscaledWidthMapping[i] = i * g_sci->_enhancementMultiplier;
		break;
	}

	_displayPixels = _displayWidth * _displayHeight;

	// Allocate visual, priority, control and display screen
	_visualScreen = (byte *)calloc(_pixels, 1);

	_visualScreenR = (byte *)calloc(_pixels, 1);
	_visualScreenG = (byte *)calloc(_pixels, 1);
	_visualScreenB = (byte *)calloc(_pixels, 1);

	_controlScreen = (byte *)calloc(_pixels, 1);
	_displayScreen = (byte *)calloc(_displayPixels, 1);
	_displayScreen_BG = (byte *)calloc(_displayPixels, 1);
	_displayScreen_R_EYE = (byte*)calloc(_displayPixels, 1);
	_displayScreen_BG_R_EYE = (byte*)calloc(_displayPixels, 1);
	_displayScreen_BGtmp = (byte *)calloc(_displayPixels, 1);
	_displayScreenR = (byte *)calloc(_displayPixels, 1);
	_displayScreenG = (byte *)calloc(_displayPixels, 1);
	_displayScreenB = (byte *)calloc(_displayPixels, 1);
	_displayScreenR_BG = (byte *)calloc(_displayPixels, 1);
	_displayScreenG_BG = (byte *)calloc(_displayPixels, 1);
	_displayScreenB_BG = (byte *)calloc(_displayPixels, 1);
	_displayScreenR_BG_R_EYE = (byte *)calloc(_displayPixels, 1);
	_displayScreenG_BG_R_EYE = (byte *)calloc(_displayPixels, 1);
	_displayScreenB_BG_R_EYE = (byte *)calloc(_displayPixels, 1);
	_displayScreenR_BGtmp = (byte *)calloc(_displayPixels, 1);
	_displayScreenG_BGtmp = (byte *)calloc(_displayPixels, 1);
	_displayScreenB_BGtmp = (byte *)calloc(_displayPixels, 1);
	memset(_displayScreenR_BGtmp, 0, _displayPixels);
	memset(_displayScreenG_BGtmp, 0, _displayPixels);
	memset(_displayScreenB_BGtmp, 0, _displayPixels);
	_displayScreenA = (byte *)calloc(_displayPixels, 1);
	_displayScreenDEPTH_IN = (byte *)calloc(_displayPixels, 1);
	_displayScreenDEPTH_SHIFT_X = (int *)calloc(_displayPixels, 4);
	_displayScreenDEPTH_SHIFT_Y = (int *)calloc(_displayPixels, 4);
	memset(_displayScreenDEPTH_IN, 0, _displayPixels);
	memset(_displayScreenDEPTH_SHIFT_X, 0, _displayPixels);
	memset(_displayScreenDEPTH_SHIFT_Y, 0, _displayPixels);
	_enhancedMatte = (byte *)calloc(_displayPixels, 1);
	_priorityScreenX = (byte *)calloc(_displayPixels, 1);
	_priorityScreenX_BG = (byte *)calloc(_displayPixels, 1);
	_priorityScreenX_BGtmp = (byte *)calloc(_displayPixels, 1);
	memset(_priorityScreenX, 0, _displayPixels);
	memset(_priorityScreenX_BG, 0, _displayPixels);
	memset(_priorityScreenX_BGtmp, 0, _displayPixels);
	_surfaceScreen = (byte *)calloc(_displayPixels, 1);

	memset(&_ditheredPicColors, 0, sizeof(_ditheredPicColors));

	// Sets display screen to be actually displayed
	_activeScreen = _displayScreen;
	_activeScreenR = _displayScreenR;
	_activeScreenG = _displayScreenG;
	_activeScreenB = _displayScreenB;

	_picNotValid = 0;
	_picNotValidSci11 = 0;
	_unditheringEnabled = true;
	_fontIsUpscaled = false;

	if (_resMan->getViewType() != kViewEga) {
		// It is not 100% accurate to set white to be 255 for Amiga 32-color
		// games. But 255 is defined as white in our SCI at all times, so it
		// doesn't matter.
		_colorWhite = 255;
		if (getSciVersion() >= SCI_VERSION_1_1)
			_colorDefaultVectorData = 255;
		else
			_colorDefaultVectorData = 0;
	} else {
		_colorWhite = 15;
		_colorDefaultVectorData = 0;
	}

	// Set up palette mods if requested
	if (ConfMan.getBool("palette_mods")) {
		setupCustomPaletteMods(this);
		ConfMan.setBool("rgb_rendering", true);
	}
	ConfMan.setBool("rgb_rendering", true);
	if (ConfMan.getBool("rgb_rendering")) {
		// Initialize the actual screen
		Graphics::PixelFormat format8 = Graphics::PixelFormat::createFormatCLUT8();
		const Graphics::PixelFormat* format = &format8;
		format = 0; // Backend's preferred mode; RGB if available

		if (g_sci->hasMacIconBar()) {
			// For SCI1.1 Mac games with the custom icon bar, we need to expand the screen
			// to accommodate for the icon bar. Of course, both KQ6 and QFG1 VGA differ in size.
			// We add 2 to the height of the icon bar to add a buffer between the screen and the
			// icon bar (as did the original interpreter).
			if (g_sci->getGameId() == GID_KQ6)
				initGraphics(_displayWidth, _displayHeight + 26 + 2, format);
			else if (g_sci->getGameId() == GID_FREDDYPHARKAS)
				initGraphics(_displayWidth, _displayHeight + 28 + 2, format);
			else
				error("Unknown SCI1.1 Mac game");
		}
		else
			initGraphics(_displayWidth, _displayHeight, format);

		_format = g_system->getScreenFormat();

		// If necessary, allocate buffers for RGB mode
		if (_format.bytesPerPixel != 1) {
			_displayedScreen = (byte*)calloc(_displayPixels, 1);
			_displayedScreenR = (byte*)calloc(_displayPixels, 1);
			_displayedScreenG = (byte*)calloc(_displayPixels, 1);
			_displayedScreenB = (byte*)calloc(_displayPixels, 1);
			_rgbScreen = (byte*)calloc(_format.bytesPerPixel * _displayPixels, 1);
			_rgbScreen_LEye = (byte*)calloc(_format.bytesPerPixel * _displayPixels, 1);
			_rgbScreen_REye = (byte*)calloc(_format.bytesPerPixel * _displayPixels, 1);
			_palette = new byte[3 * 256];

			if (_paletteModsEnabled)
				_paletteMapScreen = (byte*)calloc(_displayPixels, 1);
			else
				_paletteMapScreen = 0;
		}
		else {
			_displayedScreen = 0;
			_displayedScreenR = 0;
			_displayedScreenG = 0;
			_displayedScreenB = 0;
			_palette = 0;
			_rgbScreen = 0;
			_rgbScreen_LEye = 0;
			_rgbScreen_REye = 0;
			_paletteMapScreen = 0;
		}
	}
	_backupScreen = nullptr;
}

GfxScreen::~GfxScreen() {
	free(_visualScreen);
	free(_priorityScreen);
	free(_controlScreen);
	free(_visualScreenR);
	free(_visualScreenG);
	free(_visualScreenB);
	free(_priorityScreenX);
	free(_priorityScreenX_BG);
	free(_priorityScreenX_BGtmp);
	free(_enhancedMatte);
	free(_surfaceScreen);
	free(_displayScreen);
	free(_displayScreen_BG);
	free(_displayScreen_R_EYE);
	free(_displayScreen_BG_R_EYE);
	free(_displayScreen_BGtmp);
	free(_displayScreenR);
	free(_displayScreenG);
	free(_displayScreenB);
	free(_displayScreenR_BG);
	free(_displayScreenG_BG);
	free(_displayScreenB_BG);
	free(_displayScreenR_BG_R_EYE);
	free(_displayScreenG_BG_R_EYE);
	free(_displayScreenB_BG_R_EYE);
	free(_displayScreenR_BGtmp);
	free(_displayScreenG_BGtmp);
	free(_displayScreenB_BGtmp);
	free(_displayScreenA);
	free(_displayScreenDEPTH_IN);
	free(_displayScreenDEPTH_SHIFT_X);
	free(_displayScreenDEPTH_SHIFT_Y);
	free(_paletteMapScreen);
	free(_displayedScreen);
	free(_displayedScreenR);
	free(_displayedScreenG);
	free(_displayedScreenB);
	free(_rgbScreen);
	free(_rgbScreen_LEye);
	free(_rgbScreen_REye);

	delete[] _palette;
	delete[] _backupScreen;
}

void GfxScreen::convertToRGB(const Common::Rect& rect) {
	if (!_picNotValid) {

		if (!playingVideoCutscenes) {

			assert(_format.bytesPerPixel != 1);

			debug(10, "Background Is NOT Video");
			for (int y = rect.top; y < rect.bottom; ++y) {
				const byte* inE = _enhancedMatte + y * _displayWidth + rect.left;
				const byte* inP_BG = _displayScreen_BG + y * _displayWidth + rect.left;
				const byte* inP_BG_R_EYE = _displayScreen_BG_R_EYE + y * _displayWidth + rect.left;
				const byte* inR_BG = _displayScreenR_BG + y * _displayWidth + rect.left;
				const byte* inG_BG = _displayScreenG_BG + y * _displayWidth + rect.left;
				const byte* inB_BG = _displayScreenB_BG + y * _displayWidth + rect.left;
				const byte* inR_BG_R_EYE = _displayScreenR_BG_R_EYE + y * _displayWidth + rect.left;
				const byte* inG_BG_R_EYE = _displayScreenG_BG_R_EYE + y * _displayWidth + rect.left;
				const byte* inB_BG_R_EYE = _displayScreenB_BG_R_EYE + y * _displayWidth + rect.left;
				const byte* inP = _displayScreen + y * _displayWidth + rect.left;
				const byte* inP_R_EYE = _displayScreen_R_EYE + y * _displayWidth + rect.left;
				const byte* inR = _displayedScreenR + y * _displayWidth + rect.left;
				const byte* inG = _displayedScreenG + y * _displayWidth + rect.left;
				const byte* inB = _displayedScreenB + y * _displayWidth + rect.left;
				const byte* inA = _displayScreenA + y * _displayWidth + rect.left;

				byte* out = _rgbScreen_LEye + (y * _displayWidth + rect.left) * _format.bytesPerPixel;
				if (g_sci->stereoRightEye) {
					out = _rgbScreen_REye + (y * _displayWidth + rect.left) * _format.bytesPerPixel;
				}
				// TODO: Reduce code duplication here

				if (_format.bytesPerPixel == 2) {
					if (_paletteMapScreen) {
						const byte* mod = _paletteMapScreen + y * _displayWidth + rect.left;
						for (int x = 0; x < rect.width(); ++x) {
							byte i = *inP;
							byte i_BG = *inP_BG;
							byte i_R_EYE = *inP_R_EYE;
							byte i_BG_R_EYE = *inP_BG_R_EYE;
							byte r;
							byte g;
							byte b;
							if (g_sci->enhanced_BG || g_sci->backgroundIsVideo) {

								r = *inR_BG * ((0.003921568627451) * (255 - *inA));
								g = *inG_BG * ((0.003921568627451) * (255 - *inA));
								b = *inB_BG * ((0.003921568627451) * (255 - *inA));
								if (g_sci->stereo_pair_rendering && g_sci->stereoRightEye) {
									r = *inR_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
									g = *inG_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
									b = *inB_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
								}
								if (!*mod) {
									r += _palette[3 * i + 0] * ((0.003921568627451) * (255.0000 - (*inA))) + (*inR * ((0.003921568627451) * (*inA)));
									g += _palette[3 * i + 1] * ((0.003921568627451) * (255.0000 - (*inA))) + (*inG * ((0.003921568627451) * (*inA)));
									b += _palette[3 * i + 2] * ((0.003921568627451) * (255.0000 - (*inA))) + (*inB * ((0.003921568627451) * (*inA)));
								}
								else {
									r += MIN(_palette[3 * i + 0] * (128 + _paletteMods[*mod].r) / 128, 255) * ((0.003921568627451) * (255.0000 - (*inA))) + (*inR * ((0.003921568627451) * (*inA)));
									g += MIN(_palette[3 * i + 1] * (128 + _paletteMods[*mod].g) / 128, 255) * ((0.003921568627451) * (255.0000 - (*inA))) + (*inR * ((0.003921568627451) * (*inA)));
									b += MIN(_palette[3 * i + 2] * (128 + _paletteMods[*mod].b) / 128, 255) * ((0.003921568627451) * (255.0000 - (*inA))) + (*inR * ((0.003921568627451) * (*inA)));
								}

							}
							else {

								r = _palette[3 * i_BG + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
								g = _palette[3 * i_BG + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
								b = _palette[3 * i_BG + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								if (g_sci->stereo_pair_rendering && g_sci->stereoRightEye) {
									r = _palette[3 * i_BG_R_EYE + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
									g = _palette[3 * i_BG_R_EYE + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
									b = _palette[3 * i_BG_R_EYE + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								}
								if (!*mod) {
									r += _palette[3 * i + 0] * ((0.003921568627451) * ((*inA)));
									g += _palette[3 * i + 1] * ((0.003921568627451) * ((*inA)));
									b += _palette[3 * i + 2] * ((0.003921568627451) * ((*inA)));
								}
								else {
									r += MIN(_palette[3 * i + 0] * (128 + _paletteMods[*mod].r) / 128, 255) * ((0.003921568627451) * ((*inA)));
									g += MIN(_palette[3 * i + 1] * (128 + _paletteMods[*mod].g) / 128, 255) * ((0.003921568627451) * ((*inA)));
									b += MIN(_palette[3 * i + 2] * (128 + _paletteMods[*mod].b) / 128, 255) * ((0.003921568627451) * ((*inA)));
								}
							}
							uint16 c = (uint16)_format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);

							WRITE_UINT16(out, c);
							inE += 1;
							inP_BG += 1;
							inP_BG_R_EYE += 1;
							inR_BG += 1;
							inG_BG += 1;
							inB_BG += 1;
							inR_BG_R_EYE += 1;
							inG_BG_R_EYE += 1;
							inB_BG_R_EYE += 1;
							inP += 1;
							inP_R_EYE += 1;
							inR += 1;
							inG += 1;
							inB += 1;
							inA += 1;
							out += 2;
						}
					}
					else {
						for (int x = 0; x < rect.width(); ++x) {
							byte i = *inP;
							byte i_BG = *inP_BG;
							byte i_R_EYE = *inP_R_EYE;
							byte i_BG_R_EYE = *inP_BG_R_EYE;
							byte r;
							byte g;
							byte b;

							if (g_sci->enhanced_BG || g_sci->backgroundIsVideo) {
								r = *inR_BG * ((0.003921568627451) * (255 - *inA));
								g = *inG_BG * ((0.003921568627451) * (255 - *inA));
								b = *inB_BG * ((0.003921568627451) * (255 - *inA));
								if (g_sci->stereo_pair_rendering && g_sci->stereoRightEye) {
									r = *inR_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
									g = *inG_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
									b = *inB_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
								}
								r += _palette[3 * i + 0] * ((0.003921568627451) * (255.0000 - (*inA))) + (*inR * ((0.003921568627451) * (*inA)));
								g += _palette[3 * i + 1] * ((0.003921568627451) * (255.0000 - (*inA))) + (*inG * ((0.003921568627451) * (*inA)));
								b += _palette[3 * i + 2] * ((0.003921568627451) * (255.0000 - (*inA))) + (*inB * ((0.003921568627451) * (*inA)));
							}
							else {
								r = _palette[3 * i_BG + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
								g = _palette[3 * i_BG + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
								b = _palette[3 * i_BG + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								if (g_sci->stereo_pair_rendering && g_sci->stereoRightEye) {
									r = _palette[3 * i_BG_R_EYE + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
									g = _palette[3 * i_BG_R_EYE + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
									b = _palette[3 * i_BG_R_EYE + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								}
								r += _palette[3 * i + 0] * ((0.003921568627451) * ((*inA)));
								g += _palette[3 * i + 1] * ((0.003921568627451) * ((*inA)));
								b += _palette[3 * i + 2] * ((0.003921568627451) * ((*inA)));
							}
							uint16 c = (uint16)_format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);
							WRITE_UINT16(out, c);

							inE += 1;
							inP_BG += 1;
							inP_BG_R_EYE += 1;
							inR_BG += 1;
							inG_BG += 1;
							inB_BG += 1;
							inR_BG_R_EYE += 1;
							inG_BG_R_EYE += 1;
							inB_BG_R_EYE += 1;
							inP += 1;
							inP_R_EYE += 1;
							inR += 1;
							inG += 1;
							inB += 1;
							inA += 1;
							out += 2;
						}
					}

				}
				else {
					assert(_format.bytesPerPixel == 4);

					if (_paletteMapScreen) {
						const byte* mod = _paletteMapScreen + y * _displayWidth + rect.left;
						for (int x = 0; x < rect.width(); ++x) {
							byte i = *inP;
							byte i_BG = *inP_BG;
							byte i_R_EYE = *inP_R_EYE;
							byte i_BG_R_EYE = *inP_BG_R_EYE;
							byte r;
							byte g;
							byte b;
							if (g_sci->enhanced_BG || g_sci->backgroundIsVideo) {
								r = *inR_BG * ((0.003921568627451) * (255 - *inA));
								g = *inG_BG * ((0.003921568627451) * (255 - *inA));
								b = *inB_BG * ((0.003921568627451) * (255 - *inA));

								r += (*inR * ((0.003921568627451) * (*inA)));
								g += (*inG * ((0.003921568627451) * (*inA)));
								b += (*inB * ((0.003921568627451) * (*inA)));

							}
							else {
								r = _palette[3 * i_BG + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
								g = _palette[3 * i_BG + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
								b = _palette[3 * i_BG + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								if (g_sci->stereo_pair_rendering && g_sci->stereoRightEye) {
									r = _palette[3 * i_BG_R_EYE + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
									g = _palette[3 * i_BG_R_EYE + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
									b = _palette[3 * i_BG_R_EYE + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								}
								if (!*mod) {
									r += _palette[3 * i + 0] * ((0.003921568627451) * ((*inA)));
									g += _palette[3 * i + 1] * ((0.003921568627451) * ((*inA)));
									b += _palette[3 * i + 2] * ((0.003921568627451) * ((*inA)));
								}
								else {
									r += MIN(_palette[3 * i + 0] * (128 + _paletteMods[*mod].r) / 128, 255) * ((0.003921568627451) * ((*inA)));
									g += MIN(_palette[3 * i + 1] * (128 + _paletteMods[*mod].g) / 128, 255) * ((0.003921568627451) * ((*inA)));
									b += MIN(_palette[3 * i + 2] * (128 + _paletteMods[*mod].b) / 128, 255) * ((0.003921568627451) * ((*inA)));
								}
							}
							uint32 c = _format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);
							WRITE_UINT32(out, c);

							inE += 1;
							inP_BG += 1;
							inP_BG_R_EYE += 1;
							inR_BG += 1;
							inG_BG += 1;
							inB_BG += 1;
							inR_BG_R_EYE += 1;
							inG_BG_R_EYE += 1;
							inB_BG_R_EYE += 1;
							inP += 1;
							inP_R_EYE += 1;
							inR += 1;
							inG += 1;
							inB += 1;
							inA += 1;
							out += 4;
						}
					}
					else {
						for (int x = 0; x < rect.width(); ++x) {
							byte i = *inP;
							byte i_BG = *inP_BG;
							byte i_R_EYE = *inP_R_EYE;
							byte i_BG_R_EYE = *inP_BG_R_EYE;
							byte r;
							byte g;
							byte b;
							if (g_sci->enhanced_BG || g_sci->backgroundIsVideo) {
								r = *inR_BG * ((0.003921568627451) * (255 - *inA));
								g = *inG_BG * ((0.003921568627451) * (255 - *inA));
								b = *inB_BG * ((0.003921568627451) * (255 - *inA));
								if (g_sci->stereo_pair_rendering && g_sci->stereoRightEye) {
									r = *inR_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
									g = *inG_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
									b = *inB_BG_R_EYE * ((0.003921568627451) * (255 - *inA));
								}
								r += (*inR * ((0.003921568627451) * (*inA)));
								g += (*inG * ((0.003921568627451) * (*inA)));
								b += (*inB * ((0.003921568627451) * (*inA)));
							}
							else {
								r = _palette[3 * i_BG + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
								g = _palette[3 * i_BG + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
								b = _palette[3 * i_BG + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								if (g_sci->stereo_pair_rendering && g_sci->stereoRightEye) {
									r = _palette[3 * i_BG_R_EYE + 0] * ((0.003921568627451) * (255.0000 - (*inA)));
									g = _palette[3 * i_BG_R_EYE + 1] * ((0.003921568627451) * (255.0000 - (*inA)));
									b = _palette[3 * i_BG_R_EYE + 2] * ((0.003921568627451) * (255.0000 - (*inA)));
								}
								r += (*inR * ((0.003921568627451) * (*inA)));
								g += (*inG * ((0.003921568627451) * (*inA)));
								b += (*inB * ((0.003921568627451) * (*inA)));
							}
							uint32 c = _format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);
							WRITE_UINT32(out, c);
							inE += 1;
							inP_BG += 1;
							inP_BG_R_EYE += 1;
							inR_BG += 1;
							inG_BG += 1;
							inB_BG += 1;
							inR_BG_R_EYE += 1;
							inG_BG_R_EYE += 1;
							inB_BG_R_EYE += 1;
							inP += 1;
							inP_R_EYE += 1;
							inR += 1;
							inG += 1;
							inB += 1;
							inA += 1;
							out += 4;
						}
					}
				}
			}
		}
	}
	else {
		if (g_sci->stereoscopic) {
			if (!playingVideoCutscenes) {

				assert(_format.bytesPerPixel != 1);

				debug(10, "Background Is NOT Video");
				for (int y = rect.top; y < rect.bottom; ++y) {

					byte* out = _rgbScreen_LEye + (y * _displayWidth + rect.left) * _format.bytesPerPixel;
					if (g_sci->stereoRightEye) {
						out = _rgbScreen_REye + (y * _displayWidth + rect.left) * _format.bytesPerPixel;
					}
					// TODO: Reduce code duplication here

					if (_format.bytesPerPixel == 2) {
						if (_paletteMapScreen) {
							const byte* mod = _paletteMapScreen + y * _displayWidth + rect.left;
							for (int x = 0; x < rect.width(); ++x) {

								byte r;
								byte g;
								byte b;
								if (g_sci->enhanced_BG || g_sci->backgroundIsVideo) {

									r = 0;
									g = 0;
									b = 0;

								}
								else {

									r = 0;
									g = 0;
									b = 0;
								}
								uint16 c = (uint16)_format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);

								WRITE_UINT16(out, c);

								out += 2;
							}
						}
						else {
							for (int x = 0; x < rect.width(); ++x) {

								byte r;
								byte g;
								byte b;

								r = 0;
								g = 0;
								b = 0;
								uint16 c = (uint16)_format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);
								WRITE_UINT16(out, c);

								out += 2;
							}
						}

					}
					else {
						assert(_format.bytesPerPixel == 4);

						if (_paletteMapScreen) {
							const byte* mod = _paletteMapScreen + y * _displayWidth + rect.left;
							for (int x = 0; x < rect.width(); ++x) {

								byte r;
								byte g;
								byte b;
								r = 0;
								g = 0;
								b = 0;
								uint32 c = _format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);
								WRITE_UINT32(out, c);

								out += 4;
							}
						}
						else {
							for (int x = 0; x < rect.width(); ++x) {

								byte r;
								byte g;
								byte b;
								r = 0;
								g = 0;
								b = 0;
								uint32 c = _format.RGBToColor(r * blackFade, g * blackFade, b * blackFade);
								WRITE_UINT32(out, c);

								out += 4;
							}
						}
					}
				}
			}
		}
	}
}

void GfxScreen::displayRectRGB(const Common::Rect &rect, int x, int y) {
	// Display rect from _activeScreen to screen location x, y.
	// Clipping is assumed to be done already.

	Common::Rect targetRect;
	targetRect.left = x;
	targetRect.setWidth(rect.width());
	targetRect.top = y;
	targetRect.setHeight(rect.height());

	// 1. Update _displayedScreen
	for (int i = 0; i < rect.height(); ++i) {
		int offset = (rect.top + i) * _displayWidth + rect.left;
		int targetOffset = (targetRect.top + i) * _displayWidth + targetRect.left;
		memcpy(_displayedScreen + targetOffset, _activeScreen + offset, rect.width());
		memcpy(_displayedScreenR + targetOffset, _activeScreenR + offset, rect.width());
		memcpy(_displayedScreenG + targetOffset, _activeScreenG + offset, rect.width());
		memcpy(_displayedScreenB + targetOffset, _activeScreenB + offset, rect.width());
	}

	// 2. Convert to RGB
	convertToRGB(targetRect);

	// 3. Copy to screen
	if (!g_sci->stereoRightEye) {
		g_system->copyRectToScreen(_rgbScreen_LEye + ((targetRect.top * _displayWidth) + targetRect.left) * _format.bytesPerPixel, _displayWidth * _format.bytesPerPixel, targetRect.left, targetRect.top, targetRect.width(), targetRect.height());
	} else {
		g_system->copyRectToScreen(_rgbScreen_REye + ((targetRect.top * _displayWidth) + targetRect.left) * _format.bytesPerPixel, _displayWidth * _format.bytesPerPixel, targetRect.left, targetRect.top, targetRect.width(), targetRect.height());
	}
}

void GfxScreen::displayRect(const Common::Rect &rect, int x, int y) {
	// Display rect from _activeScreen to screen location x, y.
	// Clipping is assumed to be done already.

	if (_format.bytesPerPixel == 1) {
		g_system->copyRectToScreen(_activeScreen + rect.top * _displayWidth + rect.left, _displayWidth, x, y, rect.width(), rect.height());
	} else {
		displayRectRGB(rect, x, y);
	}
}


// should not be used regularly; only meant for restore game
void GfxScreen::clearForRestoreGame() {
	// reset all screen data
	memset(_visualScreen, 0, _pixels);
	memset(_priorityScreen, 0, _pixels);
	memset(_controlScreen, 0, _pixels);
	memset(_visualScreenR, 0, _pixels);
	memset(_visualScreenG, 0, _pixels);
	memset(_visualScreenB, 0, _pixels);
	memset(_enhancedMatte, 0, _displayPixels);
	memset(_surfaceScreen, 0, _displayPixels);
	memset(_displayScreen, 0, _displayPixels);
	memset(_displayScreen_BG, 0, _displayPixels);
	memset(_displayScreen_BGtmp, 0, _displayPixels);
	memset(_displayScreenR, 0, _displayPixels);
	memset(_displayScreenG, 0, _displayPixels);
	memset(_displayScreenB, 0, _displayPixels);
	memset(_displayScreenR_BG, 0, _displayPixels);
	memset(_displayScreenG_BG, 0, _displayPixels);
	memset(_displayScreenB_BG, 0, _displayPixels);
	memset(_displayScreenR_BG_R_EYE, 0, _displayPixels);
	memset(_displayScreenG_BG_R_EYE, 0, _displayPixels);
	memset(_displayScreenB_BG_R_EYE, 0, _displayPixels);
	memset(_displayScreenR_BGtmp, 0, _displayPixels);
	memset(_displayScreenG_BGtmp, 0, _displayPixels);
	memset(_displayScreenB_BGtmp, 0, _displayPixels);
	memset(_displayScreenA, 0, _displayPixels);
	memset(_displayScreenDEPTH_IN, 0, _displayPixels);
	memset(_displayScreenDEPTH_SHIFT_X, 0, _displayPixels);
	memset(_displayScreenDEPTH_SHIFT_Y, 0, _displayPixels);
	memset(_priorityScreenX, 0, _displayPixels);
	memset(_priorityScreenX_BG, 0, _displayPixels);
	memset(_priorityScreenX_BGtmp, 0, _displayPixels);
	if (_displayedScreen) {
		memset(_displayedScreen, 0, _displayPixels);
		memset(_displayedScreenR, 0, _displayPixels);
		memset(_displayedScreenG, 0, _displayPixels);
		memset(_displayedScreenB, 0, _displayPixels);
		memset(_rgbScreen, 0, _format.bytesPerPixel*_displayPixels);
		memset(_rgbScreen_LEye, 0, _format.bytesPerPixel * _displayPixels);
		memset(_rgbScreen_REye, 0, _format.bytesPerPixel * _displayPixels);
		if (_paletteMapScreen)
			memset(_paletteMapScreen, 0, _displayPixels);
	}
	memset(&_ditheredPicColors, 0, sizeof(_ditheredPicColors));
	_fontIsUpscaled = false;
	copyToScreen();
}

void GfxScreen::copyToScreen() {
	Common::Rect r(0, 0, _displayWidth, _displayHeight);
	displayRect(r, 0, 0);
}

void GfxScreen::copyVideoFrameToScreen(const byte *buffer, int pitch, const Common::Rect &rect, bool is8bit) {
	if (_format.bytesPerPixel == 1 || !is8bit) {
		g_system->copyRectToScreen(buffer, pitch, rect.left, rect.top, rect.width(), rect.height());
	} else {
		for (int i = 0; i < rect.height(); ++i) {
			int offset = i * pitch;
			int targetOffset = (rect.top + i) * _displayWidth + rect.left;
			memcpy(_displayedScreen + targetOffset, buffer + offset, rect.width());
		}
		convertToRGB(rect);
		if (!g_sci->stereoRightEye) {
			g_system->copyRectToScreen(_rgbScreen_LEye + (rect.top * _displayWidth + rect.left) * _format.bytesPerPixel, _displayWidth * _format.bytesPerPixel, rect.left, rect.top, rect.width(), rect.height());
		} else {
			g_system->copyRectToScreen(_rgbScreen_REye + (rect.top * _displayWidth + rect.left) * _format.bytesPerPixel, _displayWidth * _format.bytesPerPixel, rect.left, rect.top, rect.width(), rect.height());
		}
	}
}

void GfxScreen::kernelSyncWithFramebuffer() {
	if (_format.bytesPerPixel == 1) {
		Graphics::Surface *screen = g_system->lockScreen();
		const byte *pix = (const byte *)screen->getPixels();
		for (int y = 0; y < _displayHeight; ++y) {
			memcpy(_displayScreen + y * _displayWidth, pix + y * screen->pitch, _displayWidth);
			memcpy(_enhancedMatte + y * _displayWidth, pix + y * screen->pitch, _displayWidth);
		}
		g_system->unlockScreen();
	} else {
		memcpy(_enhancedMatte, _displayedScreen, _displayPixels);
		memcpy(_displayScreen, _displayedScreen, _displayPixels);
		memcpy(_displayScreenR, _displayedScreenR, _displayPixels);
		memcpy(_displayScreenG, _displayedScreenG, _displayPixels);
		memcpy(_displayScreenB, _displayedScreenB, _displayPixels);
	}
}

void GfxScreen::copyRectToScreen(const Common::Rect &rect) {
	if (!_upscaledHires)  {
		displayRect(rect, rect.left, rect.top);
	} else {
		int rectHeight = _upscaledHeightMapping[rect.bottom] - _upscaledHeightMapping[rect.top];
		int rectWidth  = _upscaledWidthMapping[rect.right] - _upscaledWidthMapping[rect.left];

		Common::Rect r;
		r.left =  _upscaledWidthMapping[rect.left];
		r.top = _upscaledHeightMapping[rect.top];
		r.setWidth(rectWidth);
		r.setHeight(rectHeight);
		displayRect(r, r.left, r.top);
	}
}

/**
 * This copies a rect to screen w/o scaling adjustment and is only meant to be
 * used on hires graphics used in upscaled hires mode.
 */
void GfxScreen::copyDisplayRectToScreen(const Common::Rect &rect) {
	if (!_upscaledHires)
		error("copyDisplayRectToScreen: not in upscaled hires mode");

	displayRect(rect, rect.left, rect.top);
}

void GfxScreen::copyRectToScreen(const Common::Rect &rect, int16 x, int16 y) {
	if (!_upscaledHires)  {
		displayRect(rect, x, y);
	} else {
		int rectHeight = _upscaledHeightMapping[rect.bottom] - _upscaledHeightMapping[rect.top];
		int rectWidth  = _upscaledWidthMapping[rect.right] - _upscaledWidthMapping[rect.left];

		Common::Rect r;
		r.left =  _upscaledWidthMapping[rect.left];
		r.top = _upscaledHeightMapping[rect.top];
		r.setWidth(rectWidth);
		r.setHeight(rectHeight);
		displayRect(r, _upscaledWidthMapping[x], _upscaledHeightMapping[y]);
	}
}

byte GfxScreen::getDrawingMask(byte color, byte prio, byte control) {
	byte flag = 0;
	if (color != 255)
		flag |= GFX_SCREEN_MASK_VISUAL;
	if (prio != 255)
		flag |= GFX_SCREEN_MASK_PRIORITY;
	if (control != 255)
		flag |= GFX_SCREEN_MASK_CONTROL;
	return flag;
}

void GfxScreen::vectorAdjustLineCoordinates(int16 *left, int16 *top, int16 *right, int16 *bottom, byte drawMask, byte color, byte priority, byte control) {
	switch (_upscaledHires) {
	case GFX_SCREEN_UPSCALED_480x300: {
		int16 displayLeft = (*left * 3) / 2;
		int16 displayRight = (*right * 3) / 2;
		int16 displayTop = (*top * 3) / 2;
		int16 displayBottom = (*bottom * 3) / 2;

		if (displayLeft < displayRight) {
			// one more pixel to the left, one more pixel to the right
			if (displayLeft > 0)
				vectorPutLinePixel(displayLeft - 1, displayTop, drawMask, color, priority, control);
			vectorPutLinePixel(displayRight + 1, displayBottom, drawMask, color, priority, control);
		} else if (displayLeft > displayRight) {
			if (displayRight > 0)
				vectorPutLinePixel(displayRight - 1, displayBottom, drawMask, color, priority, control);
			vectorPutLinePixel(displayLeft + 1, displayTop, drawMask, color, priority, control);
		}
		*left = displayLeft;
		*top = displayTop;
		*right = displayRight;
		*bottom = displayBottom;
		break;
	}
	default:
		break;
	}
}

// This is called from vector drawing to put a pixel at a certain location
void GfxScreen::vectorPutLinePixel(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control) {
	if (_upscaledHires == GFX_SCREEN_UPSCALED_480x300) {
		vectorPutLinePixel480x300(x, y, drawMask, color, priority, control);
		return;
	}

	// For anything else forward to the regular putPixel
	putPixel(x, y, drawMask, color, priority, control, true);
}

// Special 480x300 Mac putPixel for vector line drawing, also draws an additional pixel below the actual one
void GfxScreen::vectorPutLinePixel480x300(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control) {
	int offset = y * _width + x;
	int offsetPrio = ((y * g_sci->_enhancementMultiplier) * _displayWidth) + (x * g_sci->_enhancementMultiplier);
	if (drawMask & GFX_SCREEN_MASK_VISUAL) {
		// also set pixel below actual pixel
		_visualScreen[offset] = color;
		_visualScreen[offset + _width] = color;
		_displayScreen[offset] = color;
		_displayScreen[offset + _displayWidth] = color;
		_enhancedMatte[offset] = 0;
		_enhancedMatte[offset + _displayWidth] = 0;
	}
	if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
		_priorityScreenX[offsetPrio] = priority;
		_priorityScreenX[offsetPrio + _displayWidth] = priority;
	}
	if (drawMask & GFX_SCREEN_MASK_CONTROL) {
		_controlScreen[offset] = control;
		_controlScreen[offset + _width] = control;
	}
}

byte GfxScreen::vectorIsFillMatch(int16 x, int16 y, byte screenMask, byte checkForColor, byte checkForPriority, byte checkForControl, bool isEGA) {
	int offset = y * _width + x;
	int offsetPrio = ((y * g_sci->_enhancementMultiplier) * _displayWidth) + (x * g_sci->_enhancementMultiplier);
	if (_upscaledHires == GFX_SCREEN_UPSCALED_640x400) {
		offsetPrio = ((y * g_sci->_enhancementMultiplier * 2) * _displayWidth) + (x * g_sci->_enhancementMultiplier * 2);
	}
	byte match = 0;

	if (screenMask & GFX_SCREEN_MASK_VISUAL) {
		if (!isEGA) {
			if (*(_visualScreen + offset) == checkForColor)
				match |= GFX_SCREEN_MASK_VISUAL;
		} else {
			// In EGA games a pixel in the framebuffer is only 4 bits. We store
			// a full byte per pixel to allow undithering, but when comparing
			// pixels for flood-fill purposes, we should only compare the
			// visible color of a pixel.

			byte EGAcolor = *(_visualScreen + offset);
			if ((x ^ y) & 1)
				EGAcolor = (EGAcolor ^ (EGAcolor >> 4)) & 0x0F;
			else
				EGAcolor = EGAcolor & 0x0F;
			if (EGAcolor == checkForColor)
				match |= GFX_SCREEN_MASK_VISUAL;
		}
	}
	if ((screenMask & GFX_SCREEN_MASK_PRIORITY) && *(_priorityScreenX + offsetPrio) == checkForPriority)
		match |= GFX_SCREEN_MASK_PRIORITY;
	if ((screenMask & GFX_SCREEN_MASK_CONTROL) && *(_controlScreen + offset) == checkForControl)
		match |= GFX_SCREEN_MASK_CONTROL;
	return match;
}

/**
 * Sierra's Bresenham line drawing.
 * WARNING: Do not replace this with Graphics::drawLine(), as this causes issues
 * with flood fill, due to small difference in the Bresenham logic.
 */
void GfxScreen::drawLine(Common::Point startPoint, Common::Point endPoint, byte color, byte priority, byte control) {
	int16 maxWidth = _width - 1;
	int16 maxHeight = _height - 1;
	// we need to clip values here, lsl3 room 620 background picture draws a line from 0, 199 t 320, 199
	//  otherwise we would get heap corruption.
	int16 left = CLIP<int16>(startPoint.x, 0, maxWidth);
	int16 top = CLIP<int16>(startPoint.y, 0, maxHeight);
	int16 right = CLIP<int16>(endPoint.x, 0, maxWidth);
	int16 bottom = CLIP<int16>(endPoint.y, 0, maxHeight);

	//set_drawing_flag
	byte drawMask = getDrawingMask(color, priority, control);

	vectorAdjustLineCoordinates(&left, &top, &right, &bottom, drawMask, color, priority, control);

	// horizontal line
	if (top == bottom) {
		if (right < left)
			SWAP(right, left);
		for (int i = left; i <= right; i++)
			vectorPutLinePixel(i, top, drawMask, color, priority, control);
		return;
	}
	// vertical line
	if (left == right) {
		if (top > bottom)
			SWAP(top, bottom);
		for (int i = top; i <= bottom; i++)
			vectorPutLinePixel(left, i, drawMask, color, priority, control);
		return;
	}
	// sloped line - draw with Bresenham algorithm
	int16 dy = bottom - top;
	int16 dx = right - left;
	int16 stepy = dy < 0 ? -1 : 1;
	int16 stepx = dx < 0 ? -1 : 1;
	dy = ABS(dy) << 1;
	dx = ABS(dx) << 1;

	// setting the 1st and last pixel
	vectorPutLinePixel(left, top, drawMask, color, priority, control);
	vectorPutLinePixel(right, bottom, drawMask, color, priority, control);
	// drawing the line
	if (dx > dy) { // going horizontal
		int fraction = dy - (dx >> 1);
		while (left != right) {
			if (fraction >= 0) {
				top += stepy;
				fraction -= dx;
			}
			left += stepx;
			fraction += dy;
			vectorPutLinePixel(left, top, drawMask, color, priority, control);
		}
	} else { // going vertical
		int fraction = dx - (dy >> 1);
		while (top != bottom) {
			if (fraction >= 0) {
				left += stepx;
				fraction -= dy;
			}
			top += stepy;
			fraction += dx;
			vectorPutLinePixel(left, top, drawMask, color, priority, control);
		}
	}
}

// We put native Mac fonts onto an upscaled background if they are hires,
// otherwise lores Mac fonts are used with no upscaling. The caller handles
// all of this so there are no scaling adjustments to make here.
void GfxScreen::putMacChar(const Graphics::Font *commonFont, int16 x, int16 y, uint16 chr, byte color) {
	//commonFont->drawChar(&_displayScreenSurface, chr, x, y, color); // This needs to be fixed later, busy cherry picking.. (MiLO)
}

// We put hires hangul chars onto upscaled background, so we need to adjust
// coordinates. Caller gives use low-res ones.
void GfxScreen::putHangulChar(Graphics::FontKorean *commonFont, int16 x, int16 y, uint16 chr, byte color) {
	if (g_sci->enhanced_BG) {
		byte r;
		byte g;
		byte b;
		_format.colorToRGB(color, r, g, b);
		commonFont->drawChar(_displayScreenR_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, r, 0, -1, -1);
		commonFont->drawChar(_displayScreenG_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, g, 0, -1, -1);
		commonFont->drawChar(_displayScreenB_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, b, 0, -1, -1);
		commonFont->drawChar(_displayScreenR_BG_R_EYE + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, r, 0, -1, -1);
		commonFont->drawChar(_displayScreenG_BG_R_EYE + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, g, 0, -1, -1);
		commonFont->drawChar(_displayScreenB_BG_R_EYE + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, b, 0, -1, -1);
		commonFont->drawChar(_displayScreenR + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, r, 0, -1, -1);
		commonFont->drawChar(_displayScreenG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, g, 0, -1, -1);
		commonFont->drawChar(_displayScreenB + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, b, 0, -1, -1);
	} else {
		byte *displayPtr = _displayScreen + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier);
		byte *displayPtr_BG = _displayScreen_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier);
		// we don't use outline, so color 0 is actually not used
		commonFont->drawChar(displayPtr, chr, _displayWidth, 1, color, 0, -1, -1);
		commonFont->drawChar(displayPtr_BG, chr, _displayWidth, 1, color, 0, -1, -1);
	}
}

// We put hires kanji chars onto upscaled background, so we need to adjust
// coordinates. Caller gives use low-res ones.
void GfxScreen::putKanjiChar(Graphics::FontSJIS *commonFont, int16 x, int16 y, uint16 chr, byte color) {
	if (g_sci->enhanced_BG) {
		byte r;
		byte g;
		byte b;
		_format.colorToRGB(color, r, g, b);
		commonFont->drawChar(_displayScreenR_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, r, 0, -1, -1);
		commonFont->drawChar(_displayScreenG_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, g, 0, -1, -1);
		commonFont->drawChar(_displayScreenB_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, b, 0, -1, -1);
		commonFont->drawChar(_displayScreenR + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, r, 0, -1, -1);
		commonFont->drawChar(_displayScreenG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, g, 0, -1, -1);
		commonFont->drawChar(_displayScreenB + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier), chr, _displayWidth, 1, b, 0, -1, -1);
	} else {
		byte *displayPtr = _displayScreen + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier);
		byte *displayPtr_BG = _displayScreen_BG + ((y * _displayWidth * 2) * g_sci->_enhancementMultiplier) + (x * 2 * g_sci->_enhancementMultiplier);
		// we don't use outline, so color 0 is actually not used
		commonFont->drawChar(displayPtr, chr, _displayWidth, 1, color, 0, -1, -1);
		commonFont->drawChar(displayPtr_BG, chr, _displayWidth, 1, color, 0, -1, -1);
	}
}

int GfxScreen::bitsGetDataSize(Common::Rect rect, byte mask) {
	int byteCount = sizeof(rect) + sizeof(mask);
	int pixels = rect.width() * rect.height();
	if (mask & GFX_SCREEN_MASK_VISUAL) {
		byteCount += pixels; // _visualScreen
		byteCount += pixels; // _visualScreenR
		byteCount += pixels; // _visualScreenG
		byteCount += pixels; // _visualScreenB
		if (!_upscaledHires) {
			byteCount += pixels; // _enhancedMatte
			byteCount += pixels; // _surfaceScreen
			byteCount += pixels; // _displayScreen
			byteCount += pixels; // _displayScreen_BG
			byteCount += pixels; // _displayScreen_BGtmp
			byteCount += pixels; // _displayScreenR
			byteCount += pixels; // _displayScreenG
			byteCount += pixels; // _displayScreenB
			byteCount += pixels; // _displayScreenR_BG
			byteCount += pixels; // _displayScreenG_BG
			byteCount += pixels; // _displayScreenB_BG
			byteCount += pixels; // _displayScreenR_BG_R_EYE
			byteCount += pixels; // _displayScreenG_BG_R_EYE
			byteCount += pixels; // _displayScreenB_BG_R_EYE
			byteCount += pixels; // _displayScreenR_BGtmp
			byteCount += pixels; // _displayScreenG_BGtmp
			byteCount += pixels; // _displayScreenB_BGtmp
			byteCount += pixels; // _displayScreenA
			if (_paletteMapScreen)
				byteCount += pixels; // _paletteMapScreen
		} else {
			int rectHeight = _upscaledHeightMapping[rect.bottom] - _upscaledHeightMapping[rect.top];
			int rectWidth = _upscaledWidthMapping[rect.right] - _upscaledWidthMapping[rect.left];
			byteCount += rectHeight * rectWidth; // _enhancedMatte (upscaled hires)
			byteCount += rectHeight * rectWidth; // _surfaceScreen (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreen (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreen_BG (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreen_BGtmp (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenR (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenG (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenB (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenR_BG (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenG_BG (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenB_BG (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenR_BG_R_EYE (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenG_BG_R_EYE (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenB_BG_R_EYE (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenR_BGtmp (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenG_BGtmp (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenB_BGtmp (upscaled hires)
			byteCount += rectHeight * rectWidth; // _displayScreenA (upscaled hires)
			if (_paletteMapScreen)
				byteCount += rectHeight * rectWidth; // _paletteMapScreen (upscaled hires)
		}
	}
	if (mask & GFX_SCREEN_MASK_PRIORITY) {
		if (!_upscaledHires) {
			byteCount += pixels; // _priorityScreenX
			byteCount += pixels; // _priorityScreenX_BG
			byteCount += pixels; // _priorityScreenXtmp
		} else {
			int rectHeight = _upscaledHeightMapping[rect.bottom] - _upscaledHeightMapping[rect.top];
			int rectWidth = _upscaledWidthMapping[rect.right] - _upscaledWidthMapping[rect.left];
			byteCount += rectHeight * rectWidth; // _priorityScreenX (upscaled hires)
			byteCount += rectHeight * rectWidth; // _priorityScreenX_BG (upscaled hires)
			byteCount += rectHeight * rectWidth; // _priorityScreenXtmp (upscaled hires)
		}
	}
	if (mask & GFX_SCREEN_MASK_CONTROL) {
		byteCount += pixels; // _controlScreen
	}
	if (mask & GFX_SCREEN_MASK_DISPLAY) {
		if (!_upscaledHires)
			error("bitsGetDataSize() called w/o being in upscaled hires mode");
		byteCount += pixels; // _displayScreen (coordinates actually are given to us for hires displayScreen)
		byteCount += pixels; // _displayScreenR (coordinates actually are given to us for hires displayScreen)
		byteCount += pixels; // _displayScreenG (coordinates actually are given to us for hires displayScreen)
		byteCount += pixels; // _displayScreenB (coordinates actually are given to us for hires displayScreen)
		if (_paletteMapScreen)
			byteCount += pixels; // _paletteMapScreen
	}
	return byteCount;
}

void GfxScreen::bitsSave(Common::Rect rect, byte mask, byte *memoryPtr) {
	memcpy(memoryPtr, (void *)&rect, sizeof(rect)); memoryPtr += sizeof(rect);
	memcpy(memoryPtr, (void *)&mask, sizeof(mask)); memoryPtr += sizeof(mask);

	if (mask & GFX_SCREEN_MASK_VISUAL) {
		bitsSaveScreen(rect, _visualScreen, _width, memoryPtr);
		bitsSaveScreen(rect, _visualScreenR, _width, memoryPtr);
		bitsSaveScreen(rect, _visualScreenG, _width, memoryPtr);
		bitsSaveScreen(rect, _visualScreenB, _width, memoryPtr);
		bitsSaveDisplayScreen(rect, _enhancedMatte, memoryPtr);
		bitsSaveDisplayScreen(rect, _surfaceScreen, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreen, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreen_BG, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreen_BGtmp, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenR, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenG, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenB, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenR_BG, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenG_BG, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenB_BG, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenR_BG_R_EYE, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenG_BG_R_EYE, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenB_BG_R_EYE, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenR_BGtmp, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenG_BGtmp, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenB_BGtmp, memoryPtr);
		bitsSaveDisplayScreen(rect, _displayScreenA, memoryPtr);
		if (_paletteMapScreen)
			bitsSaveDisplayScreen(rect, _paletteMapScreen, memoryPtr);
	}
	if (mask & GFX_SCREEN_MASK_PRIORITY) {
		bitsSaveDisplayScreen(rect, _priorityScreenX, memoryPtr);
		bitsSaveDisplayScreen(rect, _priorityScreenX_BG, memoryPtr);
		bitsSaveDisplayScreen(rect, _priorityScreenX_BGtmp, memoryPtr);
	}
	if (mask & GFX_SCREEN_MASK_CONTROL) {
		bitsSaveScreen(rect, _controlScreen, _width, memoryPtr);
	}
	if (mask & GFX_SCREEN_MASK_DISPLAY) {
		if (!_upscaledHires)
			error("bitsSave() called w/o being in upscaled hires mode");
		bitsSaveScreen(rect, _enhancedMatte, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _surfaceScreen, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreen, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreen_BG, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreen_BGtmp, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenR, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenG, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenB, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenR_BG, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenG_BG, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenB_BG, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenR_BG_R_EYE, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenG_BG_R_EYE, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenB_BG_R_EYE, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenR_BGtmp, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenG_BGtmp, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenB_BGtmp, _displayWidth, memoryPtr);
		bitsSaveScreen(rect, _displayScreenA, _displayWidth, memoryPtr);
		if (_paletteMapScreen)
			bitsSaveScreen(rect, _paletteMapScreen, _displayWidth, memoryPtr);
	}
}

void GfxScreen::bitsSaveScreen(Common::Rect rect, const byte *screen, uint16 screenWidth, byte *&memoryPtr) {
	int width = rect.width();
	int y;

	screen += (rect.top * screenWidth) + rect.left;

	for (y = rect.top; y < rect.bottom; y++) {
		memcpy(memoryPtr, screen, width); memoryPtr += width;
		screen += screenWidth;
	}
}

void GfxScreen::bitsSaveDisplayScreen(Common::Rect rect, const byte *screen, byte *&memoryPtr) {
	int width;
	int y;

	if (!_upscaledHires) {
		width = rect.width();
		screen += (rect.top * _displayWidth) + rect.left;
	} else {
		screen += (_upscaledHeightMapping[rect.top] * _displayWidth) + _upscaledWidthMapping[rect.left];
		width = _upscaledWidthMapping[rect.right] - _upscaledWidthMapping[rect.left];
		rect.top = _upscaledHeightMapping[rect.top];
		rect.bottom = _upscaledHeightMapping[rect.bottom];
	}

	for (y = rect.top; y < rect.bottom; y++) {
		memcpy(memoryPtr, screen, width); memoryPtr += width;
		screen += _displayWidth;
	}
}

void GfxScreen::bitsGetRect(const byte *memoryPtr, Common::Rect *destRect) {
	memcpy(destRect, memoryPtr, sizeof(Common::Rect));
}

void GfxScreen::bitsRestore(const byte *memoryPtr) {
	Common::Rect rect;
	byte mask;

	memcpy((void *)&rect, memoryPtr, sizeof(rect)); memoryPtr += sizeof(rect);
	memcpy((void *)&mask, memoryPtr, sizeof(mask)); memoryPtr += sizeof(mask);

	if (mask & GFX_SCREEN_MASK_VISUAL) {
		bitsRestoreScreen(rect, memoryPtr, _visualScreen, _width);
		bitsRestoreScreen(rect, memoryPtr, _visualScreenR, _width);
		bitsRestoreScreen(rect, memoryPtr, _visualScreenG, _width);
		bitsRestoreScreen(rect, memoryPtr, _visualScreenB, _width);
		bitsRestoreDisplayScreen(rect, memoryPtr, _enhancedMatte);
		bitsRestoreDisplayScreen(rect, memoryPtr, _surfaceScreen);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreen);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreen_BG);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreen_BGtmp);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenR);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenG);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenB);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenR_BG);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenG_BG);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenB_BG);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenR_BG_R_EYE);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenG_BG_R_EYE);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenB_BG_R_EYE);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenR_BGtmp);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenG_BGtmp);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenB_BGtmp);
		bitsRestoreDisplayScreen(rect, memoryPtr, _displayScreenA);
		if (_paletteMapScreen)
			bitsRestoreDisplayScreen(rect, memoryPtr, _paletteMapScreen);
	}
	if (mask & GFX_SCREEN_MASK_PRIORITY) {
		bitsRestoreDisplayScreen(rect, memoryPtr, _priorityScreenX);
		bitsRestoreDisplayScreen(rect, memoryPtr, _priorityScreenX_BG);
		bitsRestoreDisplayScreen(rect, memoryPtr, _priorityScreenX_BGtmp);
	}
	if (mask & GFX_SCREEN_MASK_CONTROL) {
		bitsRestoreScreen(rect, memoryPtr, _controlScreen, _width);
	}
	if (mask & GFX_SCREEN_MASK_DISPLAY) {
		if (!_upscaledHires)
			error("bitsRestore() called w/o being in upscaled hires mode");
		bitsRestoreScreen(rect, memoryPtr, _enhancedMatte, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _surfaceScreen, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreen, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreen_BG, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreen_BGtmp, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenR, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenG, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenB, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenR_BG, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenG_BG, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenB_BG, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenR_BG_R_EYE, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenG_BG_R_EYE, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenB_BG_R_EYE, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenR_BGtmp, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenG_BGtmp, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenB_BGtmp, _displayWidth);
		bitsRestoreScreen(rect, memoryPtr, _displayScreenA, _displayWidth);
		if (_paletteMapScreen)
			bitsRestoreScreen(rect, memoryPtr, _paletteMapScreen, _displayWidth);

		// WORKAROUND - we are not sure what sierra is doing. If we don't do this here, portraits won't get fully removed
		//  from screen. Some lowres showBits() call is used for that and it's not covering the whole area
		//  We would need to find out inside the kq6 windows interpreter, but this here works already and seems not to have
		//  any side-effects. The whole hires is hacked into the interpreter, so maybe this is even right.
		copyDisplayRectToScreen(rect);
	}
}

void GfxScreen::bitsRestoreScreen(Common::Rect rect, const byte *&memoryPtr, byte *screen, uint16 screenWidth) {
	int width = rect.width();
	int y;

	screen += (rect.top * screenWidth) + rect.left;

	for (y = rect.top; y < rect.bottom; y++) {
		memcpy((void *) screen, memoryPtr, width); memoryPtr += width;
		screen += screenWidth;
	}
}

void GfxScreen::bitsRestoreDisplayScreen(Common::Rect rect, const byte *&memoryPtr, byte *screen) {
	int width;
	int y;

	if (!_upscaledHires) {
		screen += (rect.top * _displayWidth) + rect.left;
		width = rect.width();
	} else {
		screen += (_upscaledHeightMapping[rect.top] * _displayWidth) + _upscaledWidthMapping[rect.left];
		width = _upscaledWidthMapping[rect.right] - _upscaledWidthMapping[rect.left];
		rect.top = _upscaledHeightMapping[rect.top];
		rect.bottom = _upscaledHeightMapping[rect.bottom];
	}

	for (y = rect.top; y < rect.bottom; y++) {
		memcpy((void *) screen, memoryPtr, width); memoryPtr += width;
		screen += _displayWidth;
	}
}

void GfxScreen::setShakePos(uint16 shakeXOffset, uint16 shakeYOffset) {
	if (!_upscaledHires)
		g_system->setShakePos(shakeXOffset, shakeYOffset);
	else
		g_system->setShakePos(_upscaledWidthMapping[shakeXOffset], _upscaledHeightMapping[shakeYOffset]);
}

void GfxScreen::kernelShakeScreen(uint16 shakeCount, uint16 directions) {
	while (shakeCount--) {

		uint16 shakeXOffset = 0;
		if (directions & kShakeHorizontal) {
			shakeXOffset = 10;
		}

		uint16 shakeYOffset = 0;
		if (directions & kShakeVertical) {
			shakeYOffset = 10;
		}

		setShakePos(shakeXOffset, shakeYOffset);

		g_system->updateScreen();
		g_sci->getEngineState()->sleep(3);

		setShakePos(0, 0);

		g_system->updateScreen();
		g_sci->getEngineState()->sleep(3);
	}
}

void GfxScreen::dither(bool addToFlag) {
	int y, x;
	byte color, ditheredColor;
	byte *visualPtr = _visualScreen;
	byte *displayPtr = _displayScreen_BG;
	byte *paletteMapPtr = _paletteMapScreen;

	if (!_unditheringEnabled) {
		// Do dithering on visual and display-screen
		for (y = 0; y < _height; y++) {
			for (x = 0; x < _width; x++) {
				color = *visualPtr;
				if (color & 0xF0) {
					color ^= color << 4;
					color = ((x^y) & 1) ? color >> 4 : color & 0x0F;
					switch (_upscaledHires) {
					case GFX_SCREEN_UPSCALED_DISABLED:
					case GFX_SCREEN_UPSCALED_480x300:
					case GFX_SCREEN_UPSCALED_320x200_X_EGA: // ??
						*displayPtr = color;
						if (_paletteMapScreen)
							*paletteMapPtr = _curPaletteMapValue;
						break;
					default:
						putScaledPixelOnDisplay(x, y, color, false);
						break;
					}
					*visualPtr = color;
				}
				visualPtr++; displayPtr++; paletteMapPtr++;
			}
		}
	} else {
		if (!addToFlag)
			memset(&_ditheredPicColors, 0, sizeof(_ditheredPicColors));
		// Do dithering on visual screen and put decoded but undithered byte onto display-screen
		for (y = 0; y < _height; y++) {
			for (x = 0; x < _width; x++) {
				color = *visualPtr;
				if (color & 0xF0) {
					color ^= color << 4;
					// remember dither combination for cel-undithering
					_ditheredPicColors[color]++;
					// if decoded color wants do dither with black on left side, we turn it around
					//  otherwise the normal ega color would get used for display
					if (color & 0xF0) {
						ditheredColor = color;
					}	else {
						ditheredColor = color << 4;
					}
					switch (_upscaledHires) {
					case GFX_SCREEN_UPSCALED_DISABLED:
					case GFX_SCREEN_UPSCALED_480x300:
					case GFX_SCREEN_UPSCALED_320x200_X_EGA: // ??
						*displayPtr = ditheredColor;
						if (_paletteMapScreen)
							*paletteMapPtr = _curPaletteMapValue;
						break;
					default:
						putScaledPixelOnDisplay(x, y, ditheredColor, false);
						break;
					}
					color = ((x^y) & 1) ? color >> 4 : color & 0x0F;
					*visualPtr = color;
				}
				visualPtr++; displayPtr++; paletteMapPtr++;
			}
		}
	}
}

void GfxScreen::ditherForceDitheredColor(byte color) {
	_ditheredPicColors[color] = 256;
}

void GfxScreen::enableUndithering(bool flag) {
	_unditheringEnabled = flag;
}

int16 *GfxScreen::unditherGetDitheredBgColors() {
	if (_unditheringEnabled)
		return _ditheredPicColors;
	else
		return nullptr;
}

void GfxScreen::debugShowMap(int mapNo) {
	// We cannot really support changing maps when display screen has a different resolution than visual screen
	if ((_width != _displayWidth) || (_height != _displayHeight))
		return;

	switch (mapNo) {
	case 0:
		_activeScreen = _visualScreen;
		_activeScreenR = _visualScreenR;
		_activeScreenG = _visualScreenG;
		_activeScreenB = _visualScreenB;
		break;
	case 1:
		_activeScreen = _priorityScreenX;
		break;
	case 2:
		_activeScreen = _controlScreen;
		break;
	case 3:
		_activeScreen = _displayScreen;
		_activeScreenR = _displayScreenR;
		_activeScreenG = _displayScreenG;
		_activeScreenB = _displayScreenB;
		break;
	default:
		break;
	}
	copyToScreen();
}

void GfxScreen::scale2x(const SciSpan<const byte> &src, SciSpan<byte> &dst, int16 srcWidth, int16 srcHeight, byte bytesPerPixel) {
	assert(bytesPerPixel == 1 || bytesPerPixel == 2);
	const int newWidth = srcWidth * 2;
	const int pitch = newWidth * bytesPerPixel;
	const byte *srcPtr = src.getUnsafeDataAt(0, srcWidth * srcHeight * bytesPerPixel);
	byte *dstPtr = dst.getUnsafeDataAt(0, srcWidth * srcHeight * bytesPerPixel);

	if (bytesPerPixel == 1) {
		for (int y = 0; y < srcHeight; y++) {
			for (int x = 0; x < srcWidth; x++) {
				const byte color = *srcPtr++;
				dstPtr[0] = color;
				dstPtr[1] = color;
				dstPtr[newWidth] = color;
				dstPtr[newWidth + 1] = color;
				dstPtr += 2;
			}
			dstPtr += newWidth;
		}
	} else if (bytesPerPixel == 2) {
		for (int y = 0; y < srcHeight; y++) {
			for (int x = 0; x < srcWidth; x++) {
				const byte color = *srcPtr++;
				const byte color2 = *srcPtr++;
				dstPtr[0] = color;
				dstPtr[1] = color2;
				dstPtr[2] = color;
				dstPtr[3] = color2;
				dstPtr[pitch] = color;
				dstPtr[pitch + 1] = color2;
				dstPtr[pitch + 2] = color;
				dstPtr[pitch + 3] = color2;
				dstPtr += 4;
			}
			dstPtr += pitch;
		}
	}
}

struct UpScaledAdjust {
	GfxScreenUpscaledMode gameHiresMode;
	int numerator;
	int denominator;
};

void GfxScreen::adjustToUpscaledCoordinates(int16 &y, int16 &x) {
	if (!g_sci->stereoscopic) {
		x = _upscaledWidthMapping[x];
	} else {
		x = _upscaledWidthMapping[x] / 2;
	}
	y = _upscaledHeightMapping[y];
}

void GfxScreen::adjustBackUpscaledCoordinates(int16 &y, int16 &x) {
	if (!g_sci->stereoscopic) {
		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_480x300:
			x = (x * 4) / 6;
			y = (y * 4) / 6;
			break;
		case GFX_SCREEN_UPSCALED_640x400:
			x /= 2;
			y /= 2;
			x /= g_sci->_enhancementMultiplier;
			y /= g_sci->_enhancementMultiplier;
			break;
		case GFX_SCREEN_UPSCALED_640x440:
			x /= 2;
			y = (y * 5) / 11;
			break;
		case GFX_SCREEN_UPSCALED_640x480:
			x /= 2;
			y = (y * 5) / 12;
			break;
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA:
			x /= g_sci->_enhancementMultiplier;
			y /= g_sci->_enhancementMultiplier;
			break;
		default:
			break;
		}
	} else {
		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_480x300:
			x = (x * 4) / 6;
			y = (y * 4) / 6;
			x *= 2;
			break;
		case GFX_SCREEN_UPSCALED_640x400:
			x /= 2;
			y /= 2;
			x /= g_sci->_enhancementMultiplier;
			y /= g_sci->_enhancementMultiplier;
			x *= 2;
			break;
		case GFX_SCREEN_UPSCALED_640x440:
			x /= 2;
			y = (y * 5) / 11;
			x *= 2;
			break;
		case GFX_SCREEN_UPSCALED_640x480:
			x /= 2;
			y = (y * 5) / 12;
			x *= 2;
			break;
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA:
			x /= g_sci->_enhancementMultiplier;
			y /= g_sci->_enhancementMultiplier;
			x *= 2;
			break;
		default:
			break;
		}
	}
}

int16 GfxScreen::kernelPicNotValid(int16 newPicNotValid) {
	int16 oldPicNotValid;

	if (getSciVersion() >= SCI_VERSION_1_1) {
		oldPicNotValid = _picNotValidSci11;

		if (newPicNotValid != -1)
			_picNotValidSci11 = newPicNotValid;
	} else {
		oldPicNotValid = _picNotValid;

		if (newPicNotValid != -1)
			_picNotValid = newPicNotValid;
	}

	return oldPicNotValid;
}


void GfxScreen::grabPalette(byte *buffer, uint start, uint num) const {
	assert(start + num <= 256);
	if (_format.bytesPerPixel == 1) {
		g_system->getPaletteManager()->grabPalette(buffer, start, num);
	} else {
		memcpy(buffer, _palette + 3*start, 3*num);
	}
}

void GfxScreen::setPalette(const byte *buffer, uint start, uint num, bool update) {
	assert(start + num <= 256);
	if (_format.bytesPerPixel == 1) {
		g_system->getPaletteManager()->setPalette(buffer, start, num);
	} else {
		memcpy(_palette + 3*start, buffer, 3*num);
		if (update) {
			// directly paint from _displayedScreen, not from _activeScreen
			Common::Rect r(0, 0, _displayWidth, _displayHeight);
			convertToRGB(r);
			if (!g_sci->stereoRightEye) {
				g_system->copyRectToScreen(_rgbScreen_LEye, _displayWidth * _format.bytesPerPixel, 0, 0, _displayWidth, _displayHeight);
			} else {
				g_system->copyRectToScreen(_rgbScreen_REye, _displayWidth * _format.bytesPerPixel, 0, 0, _displayWidth, _displayHeight);
			}
		}
		// CHECKME: Inside or outside the if (update)?
		// (The !update case only happens inside transitions.)
		CursorMan.replaceCursorPalette(_palette, 0, 256);
	}
}


void GfxScreen::bakCreateBackup() {
	//assert(!_backupScreen);
	_backupScreen = new byte[_format.bytesPerPixel * _displayPixels];
	if (_format.bytesPerPixel == 1) {
		Graphics::Surface *screen = g_system->lockScreen();
		memcpy(_backupScreen, screen->getPixels(), _displayPixels);
		g_system->unlockScreen();
	} else {
		if (!g_sci->stereoRightEye) {
			memcpy(_backupScreen, _rgbScreen_LEye, _format.bytesPerPixel * _displayPixels);
		} else {
			memcpy(_backupScreen, _rgbScreen_REye, _format.bytesPerPixel * _displayPixels);
		}
			
	}
}

void GfxScreen::bakDiscard() {
	assert(_backupScreen);
	delete[] _backupScreen;
	_backupScreen = nullptr;
}

void GfxScreen::bakCopyRectToScreen(const Common::Rect &rect, int16 x, int16 y) {
	assert(_backupScreen);
	const byte *ptr = _backupScreen;
	ptr += _format.bytesPerPixel * (rect.left + rect.top * _displayWidth);
	g_system->copyRectToScreen(ptr, _format.bytesPerPixel * _displayWidth, x, y, rect.width(), rect.height());
}

void GfxScreen::setPaletteMods(const PaletteMod *mods, unsigned int count) {
	assert(count < 256);
	for (unsigned int i = 0; i < count; ++i)
		_paletteMods[i] = mods[i];

	_paletteModsEnabled = true;
}



} // End of namespace Sci
