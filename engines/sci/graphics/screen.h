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

#ifndef SCI_GRAPHICS_SCREEN_H
#define SCI_GRAPHICS_SCREEN_H

#include "sci/sci.h"
#include "sci/graphics/helpers.h"
#include "sci/graphics/view.h"

#include "graphics/font.h"
#include "graphics/sjis.h"
#include "graphics/korfont.h"
#include "graphics/pixelformat.h"
#include <time.h>

namespace Sci {

enum {
	SCI_SCREEN_UPSCALEDMAXHEIGHT = 200,
	SCI_SCREEN_UPSCALEDMAXWIDTH  = 320
};

enum GfxScreenUpscaledMode {
	GFX_SCREEN_UPSCALED_DISABLED	= 0,
	GFX_SCREEN_UPSCALED_480x300     = 1,
	GFX_SCREEN_UPSCALED_640x400		= 2,
	GFX_SCREEN_UPSCALED_640x440		= 3,
	GFX_SCREEN_UPSCALED_640x480		= 4,
	GFX_SCREEN_UPSCALED_320x200_X_VGA	= 5,
	GFX_SCREEN_UPSCALED_320x200_X_EGA = 6
};

enum GfxScreenMasks {
	GFX_SCREEN_MASK_VISUAL		= 1,
	GFX_SCREEN_MASK_PRIORITY	= 2,
	GFX_SCREEN_MASK_CONTROL		= 4,
	GFX_SCREEN_MASK_DISPLAY		= 8, // not official sierra sci, only used internally
	GFX_SCREEN_MASK_ALL			= GFX_SCREEN_MASK_VISUAL|GFX_SCREEN_MASK_PRIORITY|GFX_SCREEN_MASK_CONTROL
};

enum {
	DITHERED_BG_COLORS_SIZE = 256
};

/**
 * Screen class, actually creates 3 (4) screens internally:
 * - visual/display (for the user),
 * - priority (contains priority information) and
 * - control (contains control information).
 * Handles all operations to it and copies parts of visual/display screen to
 * the actual screen, so the user can really see it.
 */
class GfxScreen {
public:
	GfxScreen(ResourceManager *resMan);
	~GfxScreen();

	uint16 getWidth() { return _width; }
	uint16 getHeight() { return _height; }
	uint16 getScriptWidth() { return _scriptWidth; }
	uint16 getScriptHeight() { return _scriptHeight; }
	uint16 getDisplayWidth() { return _displayWidth; }
	uint16 getDisplayHeight() { return _displayHeight; }
	byte getColorWhite() { return _colorWhite; }
	byte getColorDefaultVectorData() { return _colorDefaultVectorData; }

	void clearForRestoreGame();
	void copyToScreen();
	void kernelSyncWithFramebuffer();
	void copyRectToScreen(const Common::Rect &rect);
	void copyDisplayRectToScreen(const Common::Rect &rect);
	void copyRectToScreen(const Common::Rect &rect, int16 x, int16 y);

	// functions to manipulate a backup copy of the screen (for transitions)
	void bakCreateBackup();
	void bakCopyRectToScreen(const Common::Rect &rect, int16 x, int16 y);
	void bakDiscard();

	// video frame displaying
	void copyVideoFrameToScreen(const byte *buffer, int pitch, const Common::Rect &rect, bool is8bit);

	// Vector drawing
private:
	void vectorPutLinePixel(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control);
	void vectorPutLinePixel480x300(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control);

public:
	void vectorAdjustLineCoordinates(int16 *left, int16 *top, int16 *right, int16 *bottom, byte drawMask, byte color, byte priority, byte control);
	byte vectorIsFillMatch(int16 x, int16 y, byte screenMask, byte checkForColor, byte checkForPriority, byte checkForControl, bool isEGA);

	byte getDrawingMask(byte color, byte prio, byte control);
	void drawLine(Common::Point startPoint, Common::Point endPoint, byte color, byte prio, byte control);
	void drawLine(int16 left, int16 top, int16 right, int16 bottom, byte color, byte prio, byte control) {
		drawLine(Common::Point(left, top), Common::Point(right, bottom), color, prio, control);
	}

	GfxScreenUpscaledMode getUpscaledHires() const {
		return _upscaledHires;
	}

	bool isUnditheringEnabled() const {
		return _unditheringEnabled;
	}
	void enableUndithering(bool flag);

	void putMacChar(const Graphics::Font *commonFont, int16 x, int16 y, uint16 chr, byte color);
	void putKanjiChar(Graphics::FontSJIS *commonFont, int16 x, int16 y, uint16 chr, byte color);
	void putHangulChar(Graphics::FontKorean *commonFont, int16 x, int16 y, uint16 chr, byte color);

	int bitsGetDataSize(Common::Rect rect, byte mask);
	void bitsSave(Common::Rect rect, byte mask, byte *memoryPtr);
	void bitsGetRect(const byte *memoryPtr, Common::Rect *destRect);
	void bitsRestore(const byte *memoryPtr);

	void scale2x(const SciSpan<const byte> &src, SciSpan<byte> &dst, int16 srcWidth, int16 srcHeight, byte bytesPerPixel = 1);

	void adjustToUpscaledCoordinates(int16 &y, int16 &x);
	void adjustBackUpscaledCoordinates(int16 &y, int16 &x);

	void dither(bool addToFlag);

	// Force a color combination as a dithered color
	void ditherForceDitheredColor(byte color);
	int16 *unditherGetDitheredBgColors();

	void debugShowMap(int mapNo);

	int _picNotValid; // possible values 0, 1 and 2
	int _picNotValidSci11; // another variable that is used by kPicNotValid in sci1.1

	int16 kernelPicNotValid(int16 newPicNotValid);
	void kernelShakeScreen(uint16 shakeCount, uint16 direction);

	void setFontIsUpscaled(bool isUpscaled) { _fontIsUpscaled = isUpscaled; }
	bool fontIsUpscaled() const { return _fontIsUpscaled; }

	void grabPalette(byte *buffer, uint start, uint num) const;
	void setPalette(const byte *buffer, uint start, uint num, bool update = true);

	byte getCurPaletteMapValue() const { return _curPaletteMapValue; }
	void setCurPaletteMapValue(byte val) { _curPaletteMapValue = val; }
	void setPaletteMods(const PaletteMod *mods, unsigned int count);
	bool paletteModsEnabled() const { return _paletteModsEnabled; }
	void convertToRGB(const Common::Rect &rect);
	/**
	 * This variable defines, if upscaled hires is active and what upscaled mode
	 * is used.
	 */
	GfxScreenUpscaledMode _upscaledHires;
	uint16 _width;
	uint16 _height;
	byte *_displayScreen;
	byte *_displayScreen_BG;
	byte *_displayScreen_BGtmp;
	byte *_displayScreenA;
	byte *_displayedScreenR;
	byte *_displayedScreenG;
	byte *_displayedScreenB;
	byte *_displayScreenR;
	byte *_displayScreenG;
	byte *_displayScreenB;
	byte *_displayScreenR_BGtmp;
	byte *_displayScreenG_BGtmp;
	byte *_displayScreenB_BGtmp;
	byte *_displayScreenR_BG;
	byte *_displayScreenG_BG;
	byte *_displayScreenB_BG;
	byte *_displayScreenR_BG_R_EYE;
	byte *_displayScreenG_BG_R_EYE;
	byte *_displayScreenB_BG_R_EYE;
	byte *_priorityScreenX;
	byte *_priorityScreenX_BG;
	byte *_priorityScreenX_BGtmp;
	byte *_displayScreenDEPTH_IN;
	byte *_displayScreenDEPTH_OUT;
	int *_displayScreenDEPTH_SHIFT_X;
	int *_displayScreenDEPTH_SHIFT_Y;
	byte *_displayedScreen;
	byte *_rgbScreen;
	byte *_rgbScreen_LEye;
	byte *_rgbScreen_REye;
	byte *_paletteMapScreen;

	byte *_visualScreen;
	byte *_controlScreen;

	byte *_visualScreenR;
	byte *_visualScreenG;
	byte *_visualScreenB;

	byte *_surfaceScreen;
	/**
	 * This screen is the one, where pixels are copied out of into the frame buffer.
	 * It may be 640x400 for japanese SCI1 games. SCI0 games may be undithered in here.
	 * Only read from this buffer for Save/ShowBits usage.
	 */

	// Display screen copies in r g & b format


	byte *_enhancedMatte;
	// Screens for RGB mode support

	uint _pixels;
	uint16 _scriptWidth;
	uint16 _scriptHeight;
	uint16 _displayWidth;
	uint16 _displayHeight;
	uint _displayPixels;

	Graphics::PixelFormat _format;

	bool depthInit = false;
	// depth code from https://github.com/OMeyer973/Gif3DFromDepthMap_dev/blob/master/Gif3DFromDepthMapKinect/Gif3DFromDepthMapKinect.pde
	//variables to set
	float moveAmp = 1.000f;       // default 10
	float focusPoint = 2.000f;   //default = 2; 0 = focus bg
	float depthSmoothing = 4; //ammount of blur applied to the depthMap, can reduce artifacts but creates clipping
	int dispWidth = 320;         //default 2; nb of frames beetween initial point & max amplitude (= 1/2 of total number of frames)
	int myFrameRate = 60;
	int correctionRadius = 0;     //can prevent small artifacts but can be ugly. set to 0-1 or 2 max
	//computing variables
	int totalFrames = 2 * dispWidth + 1;
	int frameId = 0;
	bool goingRight = true;
	bool rendering = true;
	int nbLayers = 10;
	int greyColor, disp;
	int f, di, dx, dy, newX, newY, dxx, maxDifX, shakeX, shakeY;
	byte pixelColor, pixelColorR, pixelColorG, pixelColorB, pixelColorPrio;

private:
	
	byte _colorWhite;
	byte _colorDefaultVectorData;

	void bitsRestoreScreen(Common::Rect rect, const byte *&memoryPtr, byte *screen, uint16 screenWidth);
	void bitsRestoreDisplayScreen(Common::Rect rect, const byte *&memoryPtr, byte *screen);
	void bitsSaveScreen(Common::Rect rect, const byte *screen, uint16 screenWidth, byte *&memoryPtr);
	void bitsSaveDisplayScreen(Common::Rect rect, const byte *screen, byte *&memoryPtr);

	void setShakePos(uint16 shakeXOffset, uint16 shakeYOffset);

	/**
	 * If this flag is true, undithering is enabled, otherwise disabled.
	 */
	bool _unditheringEnabled;
	int16 _ditheredPicColors[DITHERED_BG_COLORS_SIZE];

	// These screens have the real resolution of the game engine (320x200 for
	// SCI0/SCI1/SCI11 PC games, other resolutions on Mac). SCI0 games will be
	// dithered in here at any time.
	byte *_priorityScreen;

	// For RGB per-view/pic palette mods
	byte _curPaletteMapValue;
	PaletteMod _paletteMods[256];
	bool _paletteModsEnabled;

	byte *_backupScreen; // for bak* functions

	void displayRectRGB(const Common::Rect &rect, int x, int y);
	void displayRect(const Common::Rect &rect, int x, int y);
	byte *_palette;

	ResourceManager *_resMan;

	/**
	 * Pointer to the currently active screen (changing only required for
	 * debug purposes, to show for example the priority screen).
	 */
	byte *_activeScreen;
	byte *_activeScreenR;
	byte *_activeScreenG;
	byte *_activeScreenB;
	

	/**
	 * This here holds a translation for vertical+horizontal coordinates between native
	 * (visual) and actual (display) screen.
	 */
	int16 _upscaledHeightMapping[SCI_SCREEN_UPSCALEDMAXHEIGHT + 1];
	int16 _upscaledWidthMapping[SCI_SCREEN_UPSCALEDMAXWIDTH + 1];

	/**
	 * This defines whether or not the font we're drawing is already scaled
	 * to the screen size (and we therefore should not upscale it ourselves).
	 */
	bool _fontIsUpscaled;


	// pixel related code, in header so that it can be inlined for performance
public:
	void putPixel(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control, bool bg) {
		if (_upscaledHires == GFX_SCREEN_UPSCALED_480x300) {
			putPixel480x300(x, y, drawMask, color, priority, control);
			return;
		}

		// Set pixel for visual, priority and control map directly, those are not upscaled
		const int offset = y * _width + x;

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {
			_visualScreen[offset] = color;
			if (_paletteMapScreen)
				_paletteMapScreen[offset] = _curPaletteMapValue;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_DISABLED:
				_displayScreen[offset] = color;

				if (g_sci->backgroundIsVideo == false) {
					_enhancedMatte[offset] = 0;
				} else {
					_enhancedMatte[offset] = 128;
				}
				if (!bg) {
					_displayScreenA[offset] = 255;
					_enhancedMatte[offset] = 0;
				} else {
					_displayScreenA[offset] = 0;
				}
				break;

			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_640x440:
			case GFX_SCREEN_UPSCALED_640x480:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA: {
				putScaledPixelOnDisplay(x, y, color, false);
				break;
			}
			default:
				break;
			}
		}
		if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
			putScaledPixelInPriority(x, y, priority);
		}
		if (drawMask & GFX_SCREEN_MASK_CONTROL) {
			_controlScreen[offset] = control;
		}
	}
	void putPixel_BG(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control, bool bg) {

		if (_upscaledHires == GFX_SCREEN_UPSCALED_480x300) {
			putPixel480x300(x, y, drawMask, color, priority, control);
			return;
		}

		// Set pixel for visual, priority and control map directly, those are not upscaled
		const int offset = y * _width + x;
		if (!g_sci->stereo_pair_rendering) {
			if (drawMask & GFX_SCREEN_MASK_VISUAL) {
				_visualScreen[offset] = color;
				if (_paletteMapScreen)
					_paletteMapScreen[offset] = _curPaletteMapValue;

				switch (_upscaledHires) {
				case GFX_SCREEN_UPSCALED_DISABLED:
					_displayScreen_BG[offset] = color;

					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[offset] = 0;
					} else {
						_enhancedMatte[offset] = 128;
					}
					if (!bg) {
						_displayScreenA[offset] = 255;
						_enhancedMatte[offset] = 0;
					} else {
						_displayScreenA[offset] = 0;
					}
					break;

				case GFX_SCREEN_UPSCALED_640x400:
				case GFX_SCREEN_UPSCALED_640x440:
				case GFX_SCREEN_UPSCALED_640x480:
				case GFX_SCREEN_UPSCALED_320x200_X_VGA:
				case GFX_SCREEN_UPSCALED_320x200_X_EGA: {
					putScaledPixelOnDisplay(x, y, color, true);
					break;
				}
				default:
					break;
				}
			}
		}
		if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
			if (!g_sci->enhanced_PRIORITY)
			putScaledPixelInPriority(x, y, priority);
		}
		if (drawMask & GFX_SCREEN_MASK_CONTROL) {
			_controlScreen[offset] = control;
		}
	}
	void putPixelEtc(int16 x, int16 y, byte drawMask, byte priority, byte control) {

		// Set pixel for visual, priority and control map directly, those are not upscaled
		const int offset = y * _width + x;

		if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
			putScaledPixelInPriority(x, y, priority);
		}
		if (drawMask & GFX_SCREEN_MASK_CONTROL) {
			_controlScreen[offset] = control;
		}
	}
	void putPixelPaletted_BG(int16 x, int16 y, byte drawMask, byte c, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {
					_displayScreenA[displayOffset] = 0;
					_displayScreen_BG[displayOffset] = c;

					if (g_sci->depth_rendering)
						if (g_sci->enhanced_DEPTH)
					_displayScreen_BGtmp[displayOffset] = c;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 0;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 0;
				} else {
					//assert(_format.bytesPerPixel == 4);
					_displayScreenA[displayOffset] = 0;
					_displayScreen_BG[displayOffset] = c;

					if (g_sci->depth_rendering)
						if (g_sci->enhanced_DEPTH)
					_displayScreen_BGtmp[displayOffset] = c;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 0;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 0;
				}
				break;
			}
			default:
				break;
			}
		}
	}
	void putPixelPaletted(int16 x, int16 y, byte drawMask, byte c, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {
					_displayScreenA[displayOffset] = 255;
					_displayScreen[displayOffset] = c;
					byte r = _palette[3 * c + 0];
					byte g = _palette[3 * c + 1];
					byte b = _palette[3 * c + 2];
					_displayScreenR[displayOffset] = r;
					_displayScreenG[displayOffset] = g;
					_displayScreenB[displayOffset] = b;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 0;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 0;
				} else {
					//assert(_format.bytesPerPixel == 4);
					_displayScreenA[displayOffset] = 255;
					_displayScreen[displayOffset] = c;
					byte r = _palette[3 * c + 0];
					byte g = _palette[3 * c + 1];
					byte b = _palette[3 * c + 2];
					_displayScreenR[displayOffset] = r;
					_displayScreenG[displayOffset] = g;
					_displayScreenB[displayOffset] = b;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 0;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 0;
				}
				break;
			}
			default:
				break;
			}
		}
	}
	
	void putPixelR_BG(int16 x, int16 y, byte drawMask, byte r, byte a, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {
				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;

				if (_format.bytesPerPixel == 2) {

					
					_displayScreenA[displayOffset] = 0;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
						if (!g_sci->stereoscopic) {
							_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
						} else {
							if (g_sci->stereo_pair_rendering) {
								if (!g_sci->stereoRightEye) {
									_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								} else {
									_displayScreenR_BG_R_EYE[displayOffset] = (_displayScreenR_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								}
							} else {
								_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							}
						}
					} else {
						_enhancedMatte[displayOffset] = 128;
						if (!g_sci->stereoscopic) {
							_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
						} else {
							if (g_sci->stereo_pair_rendering) {
								if (!g_sci->stereoRightEye) {
									_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								} else {
									_displayScreenR_BG_R_EYE[displayOffset] = (_displayScreenR_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								}
							} else {
								_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							}
						}
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
				} else {
					assert(_format.bytesPerPixel == 4);

					
					_displayScreenA[displayOffset] = 0;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
						if (!g_sci->stereoscopic) {
							_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
						} else {
							if (g_sci->stereo_pair_rendering) {
								if (!g_sci->stereoRightEye) {
									_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								} else {
									_displayScreenR_BG_R_EYE[displayOffset] = (_displayScreenR_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								}
							} else {
								_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							}
						}
					} else {
						_enhancedMatte[displayOffset] = 128;
						if (!g_sci->stereoscopic) {
							_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
						} else {
							if (g_sci->stereo_pair_rendering) {
								if (!g_sci->stereoRightEye) {
									_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								} else {
									_displayScreenR_BG_R_EYE[displayOffset] = (_displayScreenR_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
								}
							} else {
								_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							}
						}
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
					
				}

				if (g_sci->depth_rendering)
				if (g_sci->enhanced_DEPTH) {
						_displayScreenR_BGtmp[displayOffset] = _displayScreenR_BG[displayOffset];
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {
					if (!g_sci->stereoscopic) {
						_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							} else {
								_displayScreenR_BG_R_EYE[displayOffset] = (_displayScreenR_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
						}
					}
					_displayScreenA[displayOffset] = 0;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
				} else {
					assert(_format.bytesPerPixel == 4);
					if (!g_sci->stereoscopic) {
						_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							} else {
								_displayScreenR_BG_R_EYE[displayOffset] = (_displayScreenR_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
						}
					}
					_displayScreenA[displayOffset] = 0;
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
				}

				if (g_sci->depth_rendering)
				if (g_sci->enhanced_DEPTH) {
					_displayScreenR_BGtmp[displayOffset] = _displayScreenR_BG[displayOffset];
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void putPixelG_BG(int16 x, int16 y, byte drawMask, byte g, byte a, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					if (!g_sci->stereoscopic) {
						_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							} else {
								_displayScreenG_BG_R_EYE[displayOffset] = (_displayScreenG_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
						}
					}
				} else {
					//assert(_format.bytesPerPixel == 4);

					if (!g_sci->stereoscopic) {
						_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							} else {
								_displayScreenG_BG_R_EYE[displayOffset] = (_displayScreenG_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
						}
					}
				}

				if (g_sci->depth_rendering)
				if (g_sci->enhanced_DEPTH) {
					_displayScreenG_BGtmp[displayOffset] = _displayScreenG_BG[displayOffset];
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					if (!g_sci->stereoscopic) {
						_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							} else {
								_displayScreenG_BG_R_EYE[displayOffset] = (_displayScreenG_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
						}
					}
				} else {
					//assert(_format.bytesPerPixel == 4);

					if (!g_sci->stereoscopic) {
						_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							} else {
								_displayScreenG_BG_R_EYE[displayOffset] = (_displayScreenG_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
						}
					}
				}

				if (g_sci->depth_rendering)
				if (g_sci->enhanced_DEPTH) {
					_displayScreenG_BGtmp[displayOffset] = _displayScreenG_BG[displayOffset];
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void putPixelB_BG(int16 x, int16 y, byte drawMask, byte b, byte a, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					if (!g_sci->stereoscopic) {
						_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							} else {
								_displayScreenB_BG_R_EYE[displayOffset] = (_displayScreenB_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
						}
					}
				} else {
					//assert(_format.bytesPerPixel == 4);

					if (!g_sci->stereoscopic) {
						_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							} else {
								_displayScreenB_BG_R_EYE[displayOffset] = (_displayScreenB_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
						}
					}
				}

				if (g_sci->depth_rendering)
				if (g_sci->enhanced_DEPTH) {
					_displayScreenB_BGtmp[displayOffset] = _displayScreenB_BG[displayOffset];
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					if (!g_sci->stereoscopic) {
						_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							} else {
								_displayScreenB_BG_R_EYE[displayOffset] = (_displayScreenB_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
						}
					}
				} else {
					//assert(_format.bytesPerPixel == 4);

					if (!g_sci->stereoscopic) {
						_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
					} else {
						if (g_sci->stereo_pair_rendering) {
							if (!g_sci->stereoRightEye) {
								_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							} else {
								_displayScreenB_BG_R_EYE[displayOffset] = (_displayScreenB_BG_R_EYE[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
							}
						} else {
							_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
						}
					}
				}

				if (g_sci->depth_rendering)
				if (g_sci->enhanced_DEPTH) {
					_displayScreenB_BGtmp[displayOffset] = _displayScreenB_BG[displayOffset];
				}
				break;
			}
			default:
				break;
			}
		}
	}
	void putPixel_DEPTH(int16 x, int16 y, byte d) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenDEPTH_IN[displayOffset] = d;

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenDEPTH_IN[displayOffset] = d;
			    }
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenDEPTH_IN[displayOffset] = d;
				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenDEPTH_IN[displayOffset] = d;
				}
				break;
			}
			default:
				break;
			}
		
	}
	int16 getDepthShift(int *screen, int16 x, int16 y) {
		int offset = (y * _displayWidth) + x;
		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_480x300: {
			offset = ((y * 3) / 2) * _width + ((y * 3) / 2);

			return screen[offset];
			break;
		}
		case GFX_SCREEN_UPSCALED_640x400: {

			offset = ((y) * (_displayWidth)) + (x);
			return screen[offset];
			break;
		}
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA: {
			offset = clip((int)((y)*_displayWidth) + ((int)(x)), 0, _displayWidth * _displayHeight - 1);

			return screen[offset];
			break;
		}
		default:
			break;
		}
		return screen[offset];
	}
	int clip(int n, int lower, int upper) {
		return std::max(lower, std::min(n, upper));
	}
	int random(int min, int max) //range : [min, max]
	{
		static bool first = true;
		if (first) {
			srand(time(NULL)); //seeding for the first time only!
			first = false;
		}
		return min + rand() % ((max + 1) - min);
	}

	void renderFrameDepthFirst(int mouseX, int mouseY) {
		if (g_sci->shakeyCam) {
			if (g_sci->stereoscopic) {
				if (!g_sci->stereoRightEye) {
					if (random(0, 5) == 4) {
						shakeX = random(0, 2) - 1;
					}
					if (random(0, 5) == 4) {
						shakeY = random(0, 2) - 1;
					}
					if (random(0, 5) == 3) {
						shakeX = random(0, 2) - 1;
						shakeY = random(0, 2) - 1;
					}
				}
			} else {
				if (random(0, 5) == 4) {
					shakeX = random(0, 2) - 1;
				}
				if (random(0, 5) == 4) {
					shakeY = random(0, 2) - 1;
				}
				if (random(0, 5) == 3) {
					shakeX = random(0, 2) - 1;
					shakeY = random(0, 2) - 1;
				}
			}
		} else {
			shakeX = 0;
			shakeY = 0;
		}
		//debug("%u", mouseX);
		dispWidth = _displayWidth / 2;
		int sizeX = _displayWidth;
		int sizeY = _displayHeight;
		int minX = sizeX;
		int maxX = 0;
		int minY = sizeY;
		int maxY = 0;
		for (di = 0; di <= 10; di++) { // 255 was too slow in 2021
			for (dy = 0; dy < sizeY; dy++) {
				//print("y : "+ y + "\n");
				bool drewPx = false;

				for (dx = 0; dx < sizeX; dx++) {
					//print("x : "+ x + "\n");
					greyColor = (int)_displayScreenDEPTH_IN[dy * sizeX + dx];

					//print("grey : " + (int)greyColor + " i : " + i +"\n");
					if ((int)(greyColor / 25.5f) >= di) { // 255 was too slow in 2021
						if (g_sci->stereoscopic) {
							if (!g_sci->stereoRightEye) {
								
								newX = clip((int)(dx + (greyColor / nbLayers - focusPoint) * (mouseX * 0.001f)) + shakeX, (int)0, (int)(sizeX - 1));
							} else {
								newX = clip((int)(dx + (greyColor / nbLayers - focusPoint) * ((mouseX + (_displayWidth * 2)) * 0.001f)) + shakeX, (int)0, (int)(sizeX - 1));
							}
						} else {
							
							newX = clip((int)(dx + (greyColor / nbLayers - focusPoint) * (mouseX * 0.0005f)) + shakeX, (int)0, (int)(sizeX - 1));
						}
						newY = clip((int)(dy + (greyColor / nbLayers - focusPoint) * (mouseY * 0.0005f)) + shakeY, (int)0, (int)(sizeY - 1));
						if (newX < minX) {
							minX = newX;
						}
						if (newX > maxX) {
							maxX = newX;
						}
						if (newY < minY) {
							minY = newY;
						}
						if (newY > maxY) {
							maxY = newY;
						}
						pixelColor = _displayScreen_BGtmp[dy * sizeX + dx];
						pixelColorR = _displayScreenR_BGtmp[dy * sizeX + dx];
						pixelColorG = _displayScreenG_BGtmp[dy * sizeX + dx];
						pixelColorB = _displayScreenB_BGtmp[dy * sizeX + dx];
						pixelColorPrio = _priorityScreenX_BGtmp[dy * sizeX + dx];
						/*
						for (dxx = clip((int)(newX - correctionRadius), 0, sizeX); dxx < clip((int)(newX + correctionRadius), 0, sizeX); dxx++) {
							//print("xx : "+ xx + "\n");
							if (greyColor > _displayScreenDEPTH_IN[dy * sizeX + dxx]) {
								_displayScreenR_BG[dy * sizeX + dxx] = pixelColorR;
								_displayScreenG_BG[dy * sizeX + dxx] = pixelColorG;
								_displayScreenB_BG[dy * sizeX + dxx] = pixelColorB;
								_priorityScreenX_BG[dy * sizeX + dxx] = pixelColorPrio;
								_displayScreenDEPTH_SHIFT[dy * sizeX + dx] = dxx;
							}
						}*/
						_displayScreen_BG[newY * sizeX + newX] = pixelColor;
						_displayScreenR_BG[newY * sizeX + newX] = pixelColorR; //(byte)((float)pixelColorR * (float)((float)clip(greyColor + 128, 128, 255)) / 255.000f);                                                      // + ((float)_displayScreenR_BG[newY * sizeX + (newX - 1)] * (1.000f - (float)((float)greyColor / 255.000f)));
						_displayScreenG_BG[newY * sizeX + newX] = pixelColorG; //(byte)((float)pixelColorG * (float)((float)clip(greyColor + 128, 128, 255)) / 255.000f); // + ((float)_displayScreenG_BG[newY * sizeX + (newX - 1)] * (1.000f - (float)((float)greyColor / 255.000f)));
						_displayScreenB_BG[newY * sizeX + newX] = pixelColorB; //(byte)((float)pixelColorB * (float)((float)clip(greyColor + 128, 128, 255)) / 255.000f); // + ((float)_displayScreenB_BG[newY * sizeX + (newX - 1)] * (1.000f - (float)((float)greyColor / 255.000f)));
						_priorityScreenX_BG[newY * sizeX + newX] = pixelColorPrio;
						_displayScreenDEPTH_SHIFT_X[dy * sizeX + dx] = newX;
						_displayScreenDEPTH_SHIFT_Y[dy * sizeX + dx] = newY;
					}
				}
			}
		}
			for (int dmy = 0; dmy < sizeY; dmy++) {
				for (int dmx = 0; dmx < sizeX; dmx++) {
					if (dmx < minX) {
						// fill left / right margins black.
						_displayScreenR_BG[dmy * sizeX + dmx] = 0;
						_displayScreenG_BG[dmy * sizeX + dmx] = 0;
						_displayScreenB_BG[dmy * sizeX + dmx] = 0;
					}
					if (dmx > maxX) {
						// fill left / right margins black.
						_displayScreenR_BG[dmy * sizeX + dmx] = 0;
						_displayScreenG_BG[dmy * sizeX + dmx] = 0;
						_displayScreenB_BG[dmy * sizeX + dmx] = 0;
					}
					if (dmy < minY) {
						// fill left / right margins black.
						_displayScreenR_BG[dmy * sizeX + dmx] = 0;
						_displayScreenG_BG[dmy * sizeX + dmx] = 0;
						_displayScreenB_BG[dmy * sizeX + dmx] = 0;
					}
					if (dmy > maxY) {
						// fill left / right margins black.
						_displayScreenR_BG[dmy * sizeX + dmx] = 0;
						_displayScreenG_BG[dmy * sizeX + dmx] = 0;
						_displayScreenB_BG[dmy * sizeX + dmx] = 0;
					}
				}
			
			}
		
		// focusPoint = (float)_displayScreenDEPTH_IN[(mouseY) * sizeX + (mouseX)] / 127.000f;
		depthInit = true;
	}
	void putPixelR(int16 x, int16 y, byte drawMask, byte r, byte a, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {
				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				
				if (_format.bytesPerPixel == 2) {

					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg) {
						if ((_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a) >= (_displayScreenA[displayOffset]))
						_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a);
						_enhancedMatte[displayOffset] = 255;
					} else {
						_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					}
				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
				
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg) {
						if ((_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a) >= (_displayScreenA[displayOffset]))
						_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a);
						_enhancedMatte[displayOffset] = 255;
					} else {
						_displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					}
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

						displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
						if (_format.bytesPerPixel == 2) {
			
							_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					       
					        if (g_sci->backgroundIsVideo == false) {
						        _enhancedMatte[displayOffset] = 255;
					        } else {
						        _enhancedMatte[displayOffset] = 128;
					        }
					        if (!bg) {
						        if (a > 1 && (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a) >= (_displayScreenA[displayOffset]))
						        _displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a);
						        _enhancedMatte[displayOffset] = 255;
					        }
						} else {
							//assert(_format.bytesPerPixel == 4);

							_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					        
					        if (g_sci->backgroundIsVideo == false) {
						        _enhancedMatte[displayOffset] = 255;
					        } else {
						        _enhancedMatte[displayOffset] = 128;
					        }
					        if (!bg) {
						        if ((_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a) >= (_displayScreenA[displayOffset]))
						        _displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a);
						        _enhancedMatte[displayOffset] = 255;
					        } else {
						        _displayScreenR_BG[displayOffset] = (_displayScreenR_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
							}
						}
				break;
			}
			default:
				break;
			}
		}
		
	}

	void putPixelG(int16 x, int16 y, byte drawMask, byte g, byte a, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
				}
				if (bg) {
					_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
				}
				if (bg) {
					_displayScreenG_BG[displayOffset] = (_displayScreenG_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void putPixelB(int16 x, int16 y, byte drawMask, byte b, byte a, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
				}
				if (bg) {
					_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
				}
				if (bg) {
					_displayScreenB_BG[displayOffset] = (_displayScreenB_BG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
				}
				break;
			}
			default:
				break;
			}
		}
	}
	void putPixelR640(int16 x, int16 y, byte drawMask, byte r, byte a, byte priority, byte control, bool bg) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {
				displayOffset = y * (_width * 2) + x;

				if (_format.bytesPerPixel == 2) {

					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255));
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
					
				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255));
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
					
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255));
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (r * ((0.003921568627451) * a));
					_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255));
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 128;
					}
					if (!bg)
						_enhancedMatte[displayOffset] = 255;
				}
				break;
			}
			default:
				break;
			}
		}
	
	}

	void putPixelG640(int16 x, int16 y, byte drawMask, byte g, byte a, byte priority, byte control) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {

				displayOffset = y * (_width * 2) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (g * ((0.003921568627451) * a));
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void putPixelB640(int16 x, int16 y, byte drawMask, byte b, byte a, byte priority, byte control) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {

				displayOffset = y * (_width * 2) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
				}
				break;
			}
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (b * ((0.003921568627451) * a));
				}
				break;
			}
			default:
				break;
			}
		}
	}
	void putFontPixelR(int16 x, int16 y, byte drawMask, byte r, byte a, byte priority, byte control) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {
					byte i = r;
					byte ir = _palette[3 * i + 0];
					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (ir * ((0.003921568627451) * a));
					
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 255;
					}
			
						if ((_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255)) >= (_displayScreenA[displayOffset]))
							_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255));
						
					
				} else {
					//assert(_format.bytesPerPixel == 4);
					byte i = r;
					byte ir = _palette[3 * i + 0];

					_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (ir * ((0.003921568627451) * a));
					
					if (g_sci->backgroundIsVideo == false) {
						_enhancedMatte[displayOffset] = 255;
					} else {
						_enhancedMatte[displayOffset] = 255;
					}
					
						if ((_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255)) >= (_displayScreenA[displayOffset]))
							_displayScreenA[displayOffset] = (_displayScreenA[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (a * ((0.003921568627451) * 255));
						_enhancedMatte[displayOffset] = 255;
					
				}

				break;
			}
			default:
				break;
			}
		}
		
	}

	void putFontPixelG(int16 x, int16 y, byte drawMask, byte g, byte a, byte priority, byte control) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {
					byte i = g;
					byte ig = _palette[3 * i + 1];
					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (ig * ((0.003921568627451) * a));

				} else {
					//assert(_format.bytesPerPixel == 4);
					byte i = g;					
					byte ig = _palette[3 * i + 1];					
					_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (ig * ((0.003921568627451) * a));
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void putFontPixelB(int16 x, int16 y, byte drawMask, byte b, byte a, byte priority, byte control) {

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {

			int displayOffset = 0;

			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

				displayOffset = (y * (_width * g_sci->_enhancementMultiplier)) + x;
				if (_format.bytesPerPixel == 2) {
					byte i = b;
					byte ib = _palette[3 * i + 2];
					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (ib * ((0.003921568627451) * a));

				} else {
					byte i = b;
					byte ib = _palette[3 * i + 2];

					_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - a))) + (ib * ((0.003921568627451) * a));
				}
				break;
			}
			default:
				break;
			}
		}
	}
	void putPixelXEtc(bool bg, int16 x, int16 y, byte drawMask, byte priority, byte control) {

		// Set pixel for visual, priority and control map directly, those are not upscaled
		const int offset = ((int)(y / g_sci->_enhancementMultiplier) * _width) + (int)(x / g_sci->_enhancementMultiplier);

		if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {
				if (bg) {
					_priorityScreenX_BG[((y) * (_width * 2)) + (x)] = priority;

					if (g_sci->depth_rendering)
					if (g_sci->enhanced_DEPTH) {
						_priorityScreenX_BGtmp[((y) * (_width * 2)) + (x)] = priority;
					}
				} else {
					_priorityScreenX[((y) * (_width * 2)) + (x)] = priority;
				}
				
				
				break;
			}
			case GFX_SCREEN_UPSCALED_640x440:
			case GFX_SCREEN_UPSCALED_640x480:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA: {
				if (bg) {
					_priorityScreenX_BG[(y * (_width * g_sci->_enhancementMultiplier)) + x] = priority;

					if (g_sci->depth_rendering)
						if (g_sci->enhanced_DEPTH) {
						_priorityScreenX_BGtmp[(y * (_width * g_sci->_enhancementMultiplier)) + x] = priority;
					}
				} else {
					_priorityScreenX[(y * (_width * g_sci->_enhancementMultiplier)) + x] = priority;

				}
				
				break;
			}
			default: {
				if (bg) {
					_priorityScreenX_BG[(y * (_width * g_sci->_enhancementMultiplier)) + x] = priority;
					if (g_sci->enhanced_DEPTH) {
						_priorityScreenX_BGtmp[(y * (_width * g_sci->_enhancementMultiplier)) + x] = priority;
					}
				} else {
					_priorityScreenX[(y * (_width * g_sci->_enhancementMultiplier)) + x] = priority;
				}
				break;
			}
			}
		}
		if (drawMask & GFX_SCREEN_MASK_CONTROL) {
			_controlScreen[offset] = control;
		}

	}
	void putPixelSurface(int16 x, int16 y, byte drawMask, byte surface) {

		// Set pixel for visual, priority and control map directly, those are not upscaled
		//const int offset = ((int)(y / g_sci->_enhancementMultiplier) * _width) + (int)(x / g_sci->_enhancementMultiplier);

		if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_640x440:
			case GFX_SCREEN_UPSCALED_640x480:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
				
					_surfaceScreen[(y * (_width * g_sci->_enhancementMultiplier)) + x] = surface;
				break;
			default:
				
					_surfaceScreen[(y * (_width * g_sci->_enhancementMultiplier)) + x] = surface;
				break;
			}
		}
	}
	void putPixel480x300(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control) {
		const int offset = ((y * 3) / 2 * _width) + ((x * 3) / 2);

		// All maps are upscaled
		// TODO: figure out, what Sierra exactly did on Mac for these games
		if (drawMask & GFX_SCREEN_MASK_VISUAL) {
			putPixel480x300Worker(x, y, offset, _visualScreen, color);
			putPixel480x300Worker(x, y, offset, _displayScreen, color);
			putPixel480x300Worker(x, y, offset, _enhancedMatte, color);	
		}
		if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
			putPixel480x300Worker(x, y, offset, _priorityScreenX, priority);
		}
		if (drawMask & GFX_SCREEN_MASK_CONTROL) {
			putPixel480x300Worker(x, y, offset, _controlScreen, control);
		}
	}
	void putPixel480x300Worker(int16 x, int16 y, int offset, byte *screen, byte byteToSet) {
		screen[offset] = byteToSet;
		if (x & 1)
			screen[offset + 1] = byteToSet;
		if (y & 1)
			screen[offset + _width] = byteToSet;
		if ((x & 1) && (y & 1))
			screen[offset + _width + 1] = byteToSet;
	}

	// This is called from vector drawing to put a pixel at a certain location
	void vectorPutPixel(int16 x, int16 y, byte drawMask, byte color, byte priority, byte control) {
		if (drawMask & GFX_SCREEN_MASK_VISUAL) {
			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_640x440:
			case GFX_SCREEN_UPSCALED_640x480:
			case GFX_SCREEN_UPSCALED_320x200_X_EGA:
			case GFX_SCREEN_UPSCALED_320x200_X_VGA:
				// For regular upscaled modes forward to the regular putPixel
				putPixel(x, y, drawMask, color, priority, control, true);
				return;
				break;

			default:
				break;
			}
		}

		// For non-upscaled mode and 480x300 Mac put pixels directly
		int offset = y * _width + x;

		if (drawMask & GFX_SCREEN_MASK_VISUAL) {
			_visualScreen[offset] = color;
			_displayScreen[offset] = color;
			_enhancedMatte[offset] = 0;
			if (_paletteMapScreen)
				_paletteMapScreen[offset] = _curPaletteMapValue;

		}
		if (drawMask & GFX_SCREEN_MASK_PRIORITY) {
			putScaledPixelInPriority(x, y, priority);
		}
		if (drawMask & GFX_SCREEN_MASK_CONTROL) {
			_controlScreen[offset] = control;
		}
	}

	/**
	 * This will just change a pixel directly on displayscreen. It is supposed to be
	 * only used on upscaled-Hires games where hires content needs to get drawn ONTO
	 * the upscaled display screen (like japanese fonts, hires portraits, etc.).
	 */
	void putPixelOnDisplay(int16 x, int16 y, byte color, bool bg) {

		int displayOffset = 0;

		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_640x400: {
			for (int yy = 0; yy < g_sci->_enhancementMultiplier * 2; yy++) {
				for (int xx = 0; xx < g_sci->_enhancementMultiplier * 2; xx++) {
					displayOffset = (((y * g_sci->_enhancementMultiplier * 2) + yy) * _displayWidth) + (x * g_sci->_enhancementMultiplier * 2) + xx; // straight 1 pixel -> 2 mapping
					if (_format.bytesPerPixel == 2) {

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];
						_displayScreenR[displayOffset] = r;
						_displayScreenG[displayOffset] = g;
						_displayScreenB[displayOffset] = b;
						_displayScreen[displayOffset] = color;
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg)
							_enhancedMatte[displayOffset] = 0;
					} else {
						assert(_format.bytesPerPixel == 4);

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];
						_displayScreenR[displayOffset] = r;
						_displayScreenG[displayOffset] = g;
						_displayScreenB[displayOffset] = b;
						_displayScreen[displayOffset] = color;
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg)
							_enhancedMatte[displayOffset] = 0;
					}
				}
			}
			break;
		}
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

			for (int yy = 0; yy < g_sci->_enhancementMultiplier; yy++) {
				for (int xx = 0; xx < g_sci->_enhancementMultiplier; xx++) {
					displayOffset = (((y * g_sci->_enhancementMultiplier) + yy) * _displayWidth) + (x * g_sci->_enhancementMultiplier) + xx; // straight 1 pixel -> 2 mapping
					if (_format.bytesPerPixel == 2) {

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];
						_displayScreenR[displayOffset] = r;
						_displayScreenG[displayOffset] = g;
						_displayScreenB[displayOffset] = b;
						_displayScreen[displayOffset] = color;
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg)
							_enhancedMatte[displayOffset] = 0;
					} else {
						assert(_format.bytesPerPixel == 4);

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];
						_displayScreenR[displayOffset] = r;
						_displayScreenG[displayOffset] = g;
						_displayScreenB[displayOffset] = b;
						_displayScreen[displayOffset] = color;
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg)
							_enhancedMatte[displayOffset] = 0;
					}
				}
			}
			break;
		}
		default:
			break;
		}
	}

	// Upscales a pixel and puts it on display screen only
	void putScaledPixelOnDisplay(int16 x, int16 y, byte color, bool bg) {
		int displayOffset = 0;

		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_640x400: {
			for (int yy = 0; yy < g_sci->_enhancementMultiplier * 2; yy++) {
				for (int xx = 0; xx < g_sci->_enhancementMultiplier * 2; xx++) {
					displayOffset = (((y * g_sci->_enhancementMultiplier * 2) + yy) * _displayWidth) + (x * g_sci->_enhancementMultiplier * 2) + xx;
					if (_format.bytesPerPixel == 2) {

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];

						
						if (!bg) {
							_displayScreen[displayOffset] = color;
							_displayScreenR[displayOffset] = r;
							_displayScreenG[displayOffset] = g;
							_displayScreenB[displayOffset] = b;
							
						} else {
							if (!g_sci->enhanced_BG) {

								_displayScreen_BG[displayOffset] = color;
								_displayScreenR_BG[displayOffset] = r;
								_displayScreenG_BG[displayOffset] = g;
								_displayScreenB_BG[displayOffset] = b;

								if (g_sci->depth_rendering)
									if (g_sci->enhanced_DEPTH) {
										_displayScreen_BGtmp[displayOffset] = color;
										_displayScreenR_BGtmp[displayOffset] = r;
										_displayScreenR_BGtmp[displayOffset] = g;
										_displayScreenR_BGtmp[displayOffset] = b;
									}
							}
						}
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg) {
							_displayScreenA[displayOffset] = 255;
							_enhancedMatte[displayOffset] = 0;
						} else {
							_displayScreenA[displayOffset] = 0;
						}

					} else if (_format.bytesPerPixel == 4) {

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];
						
						if (!bg) {
							_displayScreen[displayOffset] = color;
							_displayScreenR[displayOffset] = r;
							_displayScreenG[displayOffset] = g;
							_displayScreenB[displayOffset] = b;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreen_BG[displayOffset] = color;
								_displayScreenR_BG[displayOffset] = r;
								_displayScreenG_BG[displayOffset] = g;
								_displayScreenB_BG[displayOffset] = b;

								if (g_sci->depth_rendering)
									if (g_sci->enhanced_DEPTH) {
										_displayScreen_BGtmp[displayOffset] = color;
										_displayScreenR_BGtmp[displayOffset] = r;
										_displayScreenR_BGtmp[displayOffset] = g;
										_displayScreenR_BGtmp[displayOffset] = b;
									}
							}
						}
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg) {
							_displayScreenA[displayOffset] = 255;
							_enhancedMatte[displayOffset] = 0;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreenA[displayOffset] = 0;
							}
						}
					} else {

						byte r;
						byte g;
						byte b;
						_format.colorToRGB(color, r, g, b);
						
						if (!bg) {
							_displayScreen[displayOffset] = color;
							_displayScreenR[displayOffset] = r;
							_displayScreenG[displayOffset] = g;
							_displayScreenB[displayOffset] = b;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreen_BG[displayOffset] = color;
								_displayScreenR_BG[displayOffset] = r;
								_displayScreenG_BG[displayOffset] = g;
								_displayScreenB_BG[displayOffset] = b;

								if (g_sci->depth_rendering)
									if (g_sci->enhanced_DEPTH) {
										_displayScreen_BGtmp[displayOffset] = color;
										_displayScreenR_BGtmp[displayOffset] = r;
										_displayScreenR_BGtmp[displayOffset] = g;
										_displayScreenR_BGtmp[displayOffset] = b;
									}
							}
						}
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg) {
							_displayScreenA[displayOffset] = 255;
							_enhancedMatte[displayOffset] = 0;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreenA[displayOffset] = 0;
							}
						}
					}
				}
			}
			break;
		}
		case GFX_SCREEN_UPSCALED_640x440: {
			int16 startY = (y * 11) / 5;
			int16 endY = ((y + 1) * 11) / 5;
			displayOffset = (startY * _displayWidth) + x * 2;

			for (int16 curY = startY; curY < endY; curY++) {
				_displayScreen[displayOffset] = color;
				_displayScreen[displayOffset + 1] = color;
				_enhancedMatte[displayOffset] = 0;
				_enhancedMatte[displayOffset + 1] = 0;
				displayOffset += _displayWidth;
			}
			break;
		}
		case GFX_SCREEN_UPSCALED_640x480: {
			int16 startY = (y * 12) / 5;
			int16 endY = ((y + 1) * 12) / 5;
			displayOffset = (startY * _displayWidth) + x * 2;

			for (int16 curY = startY; curY < endY; curY++) {
				_displayScreen[displayOffset] = color;
				_displayScreen[displayOffset + 1] = color;
				_enhancedMatte[displayOffset] = 0;
				_enhancedMatte[displayOffset + 1] = 0;
				displayOffset += _displayWidth;
			}
			break;
		}
		
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

			for (int yy = 0; yy < g_sci->_enhancementMultiplier; yy++) {
				for (int xx = 0; xx < g_sci->_enhancementMultiplier; xx++) {
					displayOffset = (((y * g_sci->_enhancementMultiplier) + yy) * _displayWidth) + (x * g_sci->_enhancementMultiplier) + xx; // straight 1 pixel -> 2 mapping
					if (_format.bytesPerPixel == 2) {

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];
				
						if (!bg) {
							_displayScreen[displayOffset] = color;
							_displayScreenR[displayOffset] = r;
							_displayScreenG[displayOffset] = g;
							_displayScreenB[displayOffset] = b;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreen_BG[displayOffset] = color;
								_displayScreenR_BG[displayOffset] = r;
								_displayScreenG_BG[displayOffset] = g;
								_displayScreenB_BG[displayOffset] = b;

								if (g_sci->depth_rendering)
									if (g_sci->enhanced_DEPTH) {
										_displayScreen_BGtmp[displayOffset] = color;
										_displayScreenR_BGtmp[displayOffset] = r;
										_displayScreenR_BGtmp[displayOffset] = g;
										_displayScreenR_BGtmp[displayOffset] = b;
									}
							}
						}
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg) {
							_displayScreenA[displayOffset] = 255;
							_enhancedMatte[displayOffset] = 0;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreenA[displayOffset] = 0;
							}
						}

					} else if (_format.bytesPerPixel == 4) {

						byte i = color;
						byte r = _palette[3 * i + 0];
						byte g = _palette[3 * i + 1];
						byte b = _palette[3 * i + 2];
						
						if (!bg) {
							_displayScreen[displayOffset] = color;
							_displayScreenR[displayOffset] = r;
							_displayScreenG[displayOffset] = g;
							_displayScreenB[displayOffset] = b;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreen_BG[displayOffset] = color;
								_displayScreenR_BG[displayOffset] = r;
								_displayScreenG_BG[displayOffset] = g;
								_displayScreenB_BG[displayOffset] = b;

								if (g_sci->depth_rendering)
									if (g_sci->enhanced_DEPTH) {
										_displayScreen_BGtmp[displayOffset] = color;
										_displayScreenR_BGtmp[displayOffset] = r;
										_displayScreenR_BGtmp[displayOffset] = g;
										_displayScreenR_BGtmp[displayOffset] = b;
									}
							}
						}	
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg) {
							_displayScreenA[displayOffset] = 255;
							_enhancedMatte[displayOffset] = 0;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreenA[displayOffset] = 0;
							}
						}
					} else {

						byte r;
						byte g;
						byte b;
						_format.colorToRGB(color, r, g, b);
						
						if (!bg) {
							_displayScreen[displayOffset] = color;
							_displayScreenR[displayOffset] = r;
							_displayScreenG[displayOffset] = g;
							_displayScreenB[displayOffset] = b;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreen_BG[displayOffset] = color;
								_displayScreenR_BG[displayOffset] = r;
								_displayScreenG_BG[displayOffset] = g;
								_displayScreenB_BG[displayOffset] = b;

								if (g_sci->depth_rendering)
									if (g_sci->enhanced_DEPTH) {
										_displayScreen_BGtmp[displayOffset] = color;
										_displayScreenR_BGtmp[displayOffset] = r;
										_displayScreenR_BGtmp[displayOffset] = g;
										_displayScreenR_BGtmp[displayOffset] = b;
									}
							}
						}
						if (g_sci->backgroundIsVideo == false) {
							_enhancedMatte[displayOffset] = 0;
						} else {
							_enhancedMatte[displayOffset] = 128;
						}
						if (!bg) {
							_displayScreenA[displayOffset] = 255;
							_enhancedMatte[displayOffset] = 0;
						} else {
							if (!g_sci->enhanced_BG) {
								_displayScreenA[displayOffset] = 0;
							}
						}
					}
				}
			}
			break;
		}
		default:
			break;
		}
	}

	// Upscales a pixel and puts it on display screen only
	void putScaledPixelInPriority(int16 x, int16 y, byte priority) {
		int priorityOffset = 0;

		switch (_upscaledHires) {

		case GFX_SCREEN_UPSCALED_640x440: {
			int16 startY = (y * 11) / 5;
			int16 endY = ((y + 1) * 11) / 5;
			priorityOffset = (startY * _displayWidth) + x * 2;

			for (int16 curY = startY; curY < endY; curY++) {
				_priorityScreenX[priorityOffset] = priority;
				_priorityScreenX[priorityOffset + 1] = priority;
				if (g_sci->depth_rendering)
					if (g_sci->enhanced_DEPTH) {
						_priorityScreenX_BGtmp[priorityOffset] = priority;
						_priorityScreenX_BGtmp[priorityOffset + 1] = priority;
					}
				priorityOffset += _displayWidth;
			}
			break;
		}
		case GFX_SCREEN_UPSCALED_640x480: {
			int16 startY = (y * 12) / 5;
			int16 endY = ((y + 1) * 12) / 5;
			priorityOffset = (startY * _displayWidth) + x * 2;

			for (int16 curY = startY; curY < endY; curY++) {
				_priorityScreenX[priorityOffset] = priority;
				_priorityScreenX[priorityOffset + 1] = priority;
				if (g_sci->depth_rendering)
					if (g_sci->enhanced_DEPTH) {
						_priorityScreenX_BGtmp[priorityOffset] = priority;
						_priorityScreenX_BGtmp[priorityOffset + 1] = priority;
					}
				priorityOffset += _displayWidth;
			}
			break;
		}
		case GFX_SCREEN_UPSCALED_640x400: {
			for (int yy = 0; yy < g_sci->_enhancementMultiplier * 2; yy++) {
				for (int xx = 0; xx < g_sci->_enhancementMultiplier * 2; xx++) {
					for (int yyy = 0; yyy < g_sci->_enhancementMultiplier; yyy++) {
						for (int xxx = 0; xxx < g_sci->_enhancementMultiplier; xxx++) {
							priorityOffset = (((y * g_sci->_enhancementMultiplier * 2) + yy) * _displayWidth) + (x * g_sci->_enhancementMultiplier * 2) + xx;

							if (priorityOffset < _displayWidth * _displayHeight - 1) {
								if (_displayScreenA[priorityOffset] != 0) {
									_priorityScreenX[priorityOffset] = priority;
									if (g_sci->depth_rendering)
										if (g_sci->enhanced_DEPTH) {
											_priorityScreenX_BGtmp[priorityOffset] = priority;
										}
								}
							}
						}
					}
				}
			}
			break;
		}
		
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA: {

			for (int yy = 0; yy < g_sci->_enhancementMultiplier; yy++) {
				for (int xx = 0; xx < g_sci->_enhancementMultiplier; xx++) {
					priorityOffset = (((y * g_sci->_enhancementMultiplier) + yy) * _displayWidth) + (x * g_sci->_enhancementMultiplier) + xx;
						_priorityScreenX[priorityOffset] = priority;
					
						if (g_sci->depth_rendering)
							if (g_sci->enhanced_DEPTH) {
							    _priorityScreenX_BGtmp[priorityOffset] = priority;
							}					
				}
			}
			break;
		}
		default:
			break;
		}
	}

	/**
	 * This is used to put font pixels onto the screen - we adjust differently, so that we won't
	 *  do triple pixel lines in any case on upscaled hires. That way the font will not get distorted
	 *  Sierra SCI didn't do this
	 */
	void putFontPixel(int16 startingY, int16 x, int16 y, byte color) {
		int16 actualY = startingY + y;
		if (_fontIsUpscaled) {
			// Do not scale ourselves, but put it on the display directly
			switch (_upscaledHires) {
				case GFX_SCREEN_UPSCALED_640x400: {
					putScaledPixelOnDisplay(x, actualY, color, false);
					break;
				}
				default: 
					putPixelOnDisplay(x, actualY, color, false);
					break;
			}
		} else {
			if (_upscaledHires == GFX_SCREEN_UPSCALED_480x300) {
				putPixel480x300(x, actualY, GFX_SCREEN_MASK_VISUAL, color, 0, 0);
				return;
			}

			int offset = actualY * _width + x;

			_visualScreen[offset] = color;
			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_DISABLED:
				_displayScreen[offset] = color;
				break;
			case GFX_SCREEN_UPSCALED_640x400:
			case GFX_SCREEN_UPSCALED_640x440: {
				// to 1-> 4 pixels upscaling for all of those, so that fonts won't look weird
				int displayOffset = (_upscaledHeightMapping[startingY] + y * 2) * _displayWidth + x * 2;
				_displayScreen[displayOffset] = color;
				_displayScreen[displayOffset + 1] = color;
				displayOffset += _displayWidth;
				_displayScreen[displayOffset] = color;
				_displayScreen[displayOffset + 1] = color;
				_enhancedMatte[displayOffset] = color;
				_enhancedMatte[displayOffset + 1] = color;
				break;
			}
			default:
				putScaledPixelOnDisplay(x, actualY, color, false);
				break;
			}
		}
	}
	void putFontPixelX(int16 startingY, int16 x, int16 y, byte color, byte R, byte G, byte B, byte A) {
		int16 actualY = (int)((startingY + y) / g_sci->_enhancementMultiplier);

		if (_fontIsUpscaled) {
			// Do not scale ourselves, but put it on the display directly
			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_640x400: {
				putScaledPixelOnDisplay(x, actualY, color, false);
				break;
			}
			default:
				putPixelOnDisplay(x, actualY, color, false);
				break;
			}
		} else {
			if (_upscaledHires == GFX_SCREEN_UPSCALED_480x300) {
				putPixel480x300(x, actualY, GFX_SCREEN_MASK_VISUAL, color, 0, 0);
				return;
			}

			int offset = actualY * _width + x;

			_visualScreen[offset] = color;
			switch (_upscaledHires) {
			case GFX_SCREEN_UPSCALED_DISABLED:
				_displayScreen[offset] = color;
				break;
			case GFX_SCREEN_UPSCALED_640x400: {
				// to 1-> 4 pixels upscaling for all of those, so that fonts won't look weird
				int displayOffset = (_upscaledHeightMapping[startingY] + y * 2 * g_sci->_enhancementMultiplier) * _displayWidth + x * 2 * g_sci->_enhancementMultiplier;
				byte i = color;
				byte r = _palette[3 * i + 0];
				byte g = _palette[3 * i + 1];
				byte b = _palette[3 * i + 2];
				_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset] = color;
				_enhancedMatte[displayOffset] = 255;
				_displayScreenR[displayOffset + 1] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset + 1] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset + 1] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset + 1] = color;
				_enhancedMatte[displayOffset + 1] = 255;
				displayOffset += _displayWidth;
				_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset] = color;
				_enhancedMatte[displayOffset] = 255;
				_displayScreenR[displayOffset + 1] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset + 1] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset + 1] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset + 1] = color;
				_enhancedMatte[displayOffset + 1] = 255;
				break;
			}
			case GFX_SCREEN_UPSCALED_640x440:
			case GFX_SCREEN_UPSCALED_640x480: {
				// to 1-> 4 pixels upscaling for all of those, so that fonts won't look weird
				int displayOffset = (_upscaledHeightMapping[startingY] + y * 2) * _displayWidth + x * 2;
				byte i = color;
				byte r = _palette[3 * i + 0];
				byte g = _palette[3 * i + 1];
				byte b = _palette[3 * i + 2];
				_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset] = color;
				_enhancedMatte[displayOffset] = 255;
				_displayScreenR[displayOffset + 1] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset + 1] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset + 1] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));				
				_displayScreen[displayOffset + 1] = color;
				_enhancedMatte[displayOffset + 1] = 255;
				displayOffset += _displayWidth;
				_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset] = color;
				_enhancedMatte[displayOffset] = 255;
				_displayScreenR[displayOffset + 1] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset + 1] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset + 1] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset + 1] = color;
				_enhancedMatte[displayOffset + 1] = 255;
				break;
			}

			default:
				actualY = startingY + y;
				int16 displayOffset = (actualY * (_width * g_sci->_enhancementMultiplier)) + x;
				byte i = color;
				byte r = _palette[3 * i + 0];
				byte g = _palette[3 * i + 1];
				byte b = _palette[3 * i + 2];
				_displayScreenR[displayOffset] = (_displayScreenR[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (r * ((0.003921568627451) * A));
				_displayScreenG[displayOffset] = (_displayScreenG[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (g * ((0.003921568627451) * A));
				_displayScreenB[displayOffset] = (_displayScreenB[displayOffset] * ((0.003921568627451) * (255.0000 - A))) + (b * ((0.003921568627451) * A));
				_displayScreen[displayOffset] = color;
				_enhancedMatte[displayOffset] = 255;
				break;
			}
		}
	}

	byte getPixel(byte *screen, int16 x, int16 y) {
		int offset = (y * _width) + x;
		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_480x300: {
			offset = ((y * 3) / 2) * _width + ((y * 3) / 2);

			return screen[offset];
			break;
		}
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA: {
			offset = (int)((y) * _width) + ((int)(x));

			return screen[offset];
			break;
		}
		default:
			break;
		}
		return screen[offset];
	}

	byte getPixelX(byte *screen, int16 x, int16 y) {
		int offset = (y * _displayWidth) + x;
		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_480x300: {
			offset = ((y * 3) / 2) * _width + ((y * 3) / 2);

			return screen[offset];
			break;
		}
		case GFX_SCREEN_UPSCALED_640x400: {
			
			offset = ((y) * (_displayWidth)) + (x);
			return screen[offset];
			break;
		}
		case GFX_SCREEN_UPSCALED_320x200_X_EGA:
		case GFX_SCREEN_UPSCALED_320x200_X_VGA: {
			offset = (int)((y)*_displayWidth) + ((int)(x));

			return screen[offset];
			break;
		}
		default:
			break;
		}
		return screen[offset];
	}
	byte getVisual(int16 x, int16 y) {
		return getPixel(_visualScreen, x, y);
	}
	byte getPriorityX(int16 x, int16 y) {
		
			if (getPixelX(_priorityScreenX, (int)(x), (int)(y)) > getPixelX(_priorityScreenX_BG, (int)(x), (int)(y))) {
				return getPixelX(_priorityScreenX, (int)(x), (int)(y)); //
			} else {
				return getPixelX(_priorityScreenX_BG, (int)(x), (int)(y));
			}
		
	}
	byte getSurface(int16 x, int16 y) {
		return getPixelX(_surfaceScreen, (int)(x), (int)(y)); //
	}
	byte getPriority(int16 x, int16 y) {
		
		return getPixel(_priorityScreenX, x, y);
	}
	byte getControl(int16 x, int16 y) {
		return getPixel(_controlScreen, x, y);
	}

	// Vector related public code - in here, so that it can be inlined
	byte vectorGetPixel(byte *screen, int16 x, int16 y) {
		return screen[y * _width + x];
	}

	byte vectorGetVisual(int16 x, int16 y) {
		return vectorGetPixel(_visualScreen, x, y);
	}
	byte vectorGetPriority(int16 x, int16 y) {
		return vectorGetPixel(_priorityScreenX, x * g_sci->_enhancementMultiplier * 2, y * g_sci->_enhancementMultiplier * 2); //idk
	}
	byte vectorGetControl(int16 x, int16 y) {
		return vectorGetPixel(_controlScreen, x, y);
	}

	void vectorAdjustCoordinate(int16 *x, int16 *y) {
		switch (_upscaledHires) {
		case GFX_SCREEN_UPSCALED_480x300:
			*x = (*x * 3) / 2;
			*y = (*y * 3) / 2;
			break;
		default:
			break;
		}
	}
};

} // End of namespace Sci

#endif
