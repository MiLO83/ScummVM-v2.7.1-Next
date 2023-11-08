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

#include "common/util.h"
#include "common/stack.h"
#include "graphics/primitives.h"

#include "sci/console.h"
#include "sci/sci.h"
#include "sci/event.h"
#include "sci/engine/kernel.h"
#include "sci/engine/script_patches.h"
#include "sci/engine/state.h"
#include "sci/engine/selector.h"
#include "sci/engine/vm.h"
#include "sci/graphics/cache.h"
#include "sci/graphics/compare.h"
#include "sci/graphics/cursor.h"
#include "sci/graphics/ports.h"
#include "sci/graphics/paint16.h"
#include "sci/graphics/palette.h"
#include "sci/graphics/view.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/transitions.h"
#include "sci/graphics/animate.h"
#include <common/config-manager.h>
#include <image/png.h>
#include <map>
#include <string>
#include <cctype>
#include <common/system.h>
#include <list>
#include <algorithm>
#include <engines/sci/sound/midiparser_sci.h>
#include <dirent.h>

namespace Sci {
bool cachedViews = false;
extern bool playingVideoCutscenes;
extern bool wasPlayingVideoCutscenes;
extern std::string videoCutsceneEnd;
extern std::string videoCutsceneStart;
extern MidiParser_SCI *midiMusic;
extern bool cutscene_mute_midi;
extern std::list<std::string> extraDIRList;
extern std::list<std::string>::iterator extraDIRListit;
extern std::string extraPath;
GfxAnimate::GfxAnimate(EngineState *state, ScriptPatcher *scriptPatcher, GfxCache *cache, GfxPorts *ports, GfxPaint16 *paint16, GfxScreen *screen, GfxPalette *palette, GfxCursor *cursor, GfxTransitions *transitions)
	: _s(state), _scriptPatcher(scriptPatcher), _cache(cache), _ports(ports), _paint16(paint16), _screen(screen), _palette(palette), _cursor(cursor), _transitions(transitions) {
	init();
}

GfxAnimate::~GfxAnimate() {
}
extern std::map<std::string, std::pair<Graphics::Surface *, const byte *> > fontsMap;
extern std::map<std::string, std::pair<Graphics::Surface *, const byte *> >::iterator fontsMapit;
extern std::map<std::string, std::pair<Graphics::Surface *, const byte *> > viewsMap;
extern std::map<std::string, std::pair<Graphics::Surface *, const byte *> >::iterator viewsMapit;
extern bool preLoadedPNGs;

int clip(int n, int lower, int upper) {
	return n <= lower ? lower : n >= upper ? upper : n;
}

void GfxAnimate::init() {
	_lastCastData.clear();

	_fastCastEnabled = false;
	if (getSciVersion() == SCI_VERSION_1_1) {
		// Seems to have been available for all SCI1.1 games
		_fastCastEnabled = true;
	} else if (getSciVersion() >= SCI_VERSION_1_EARLY) {
		// fastCast only exists for some games between SCI1 early and SCI1 late
		// Try to detect it by code signature
		// It's extremely important, that we only enable it for games that actually need it
		if (detectFastCast()) {
			_fastCastEnabled = true;
		}
	}
}

// Signature for fastCast detection
static const uint16 fastCastSignature[] = {
	SIG_MAGICDWORD,
	0x35, 0x00,                      // ldi 00
	0xa1, 84,                        // sag global[84d]
	SIG_END
};

// Fast cast in games:

// SCI1 Early:
// KQ5 - no fastcast, LSL1 (demo) - no fastcast, Mixed Up Fairy Tales - *has fastcast*, XMas Card 1990 - no fastcast,
// SQ4Floppy - no fastcast, Mixed Up Mother Goose - no fastcast
//
// SCI1 Middle:
// LSL5 demo - no fastfast, Conquest of the Longbow demo - no fastcast, LSL1 - no fastcast,
// Astro Chicken II - no fastcast
//
// SCI1 Late:
// Castle of Dr. Brain demo - has fastcast, Castle of Dr. Brain - has fastcast,
// Conquests of the Longbow - has fastcast, Space Quest 1 EGA - has fastcast,
// King's Quest 5 multilingual - *NO* fastcast, Police Quest 3 demo - *NO* fastcast,
// LSL5 multilingual - has fastcast, Police Quest 3 - has fastcast,
// EcoQuest 1 - has fastcast, Mixed Up Fairy Tales demo - has fastcast,
// Space Quest 4 multilingual - *NO* fastcast
//
// SCI1.1
// Quest for Glory 3 demo - has fastcast, Police Quest 1 - hast fastcast, Quest for Glory 1 - has fastcast
// Laura Bow 2 Floppy - has fastcast, Mixed Up Mother Goose - has fastcast, Quest for Glory 3 - has fastcast
// Island of Dr. Brain - has fastcast, King's Quest 6 - has fastcast, Space Quest 5 - has fastcast
// Hoyle 4 - has fastcast, Laura Bow 2 CD - has fastcast, Freddy Pharkas CD - has fastcast
bool GfxAnimate::detectFastCast() {
	SegManager *segMan = _s->_segMan;
	const reg_t gameVMObject = g_sci->getGameObject();
	reg_t gameSuperVMObject = segMan->getObject(gameVMObject)->getSuperClassSelector();
	uint32 magicDWord = 0; // platform-specific BE/LE for performance
	int    magicDWordOffset = 0;

	if (gameSuperVMObject.isNull()) {
		gameSuperVMObject = gameVMObject; // Just in case. According to sci.cpp this may happen in KQ5CD, when loading saved games before r54510
	}

	Script *objectScript = segMan->getScript(gameSuperVMObject.getSegment());
	byte *scriptData = const_cast<byte *>(objectScript->getBuf(0));
	uint32 scriptSize = objectScript->getBufSize();

	_scriptPatcher->calculateMagicDWordAndVerify("fast cast detection", fastCastSignature, true, magicDWord, magicDWordOffset);

	// Signature is found for multilingual King's Quest 5 too, but it looks as if the fast cast global is never set
	// within that game. Which means even though we detect it as having the capability, it's never actually used.
	// The original multilingual KQ5 interpreter did have this feature disabled.
	// Sierra probably used latest system scripts and that's why we detect it.
	if (_scriptPatcher->findSignature(magicDWord, magicDWordOffset, fastCastSignature, "fast cast detection", SciSpan<const byte>(scriptData, scriptSize)) >= 0) {
		// Signature found, game seems to use fast cast for kAnimate
		return true;
	}
	return false;
}

void GfxAnimate::disposeLastCast() {
	_lastCastData.clear();
}
bool fileIsInExtraDIR(std::string fileName) {
	extraDIRListit = std::find(extraDIRList.begin(), extraDIRList.end(), fileName);
	// Check if iterator points to end or not
	if (extraDIRListit != extraDIRList.end()) {
		return true;
	} else {
		return false;
	}
}

bool GfxAnimate::invoke(List *list, int argc, reg_t *argv) {
	reg_t curAddress = list->first;
	Node *curNode = _s->_segMan->lookupNode(curAddress);
	reg_t curObject;
	uint16 signal;

	while (curNode) {
		curObject = curNode->value;

		if (_fastCastEnabled) {
			// Check if the game has a fastCast object set
			//  if we don't abort kAnimate processing, at least in kq5 there will be animation cels drawn into speech boxes.
			if (!_s->variables[VAR_GLOBAL][kGlobalVarFastCast].isNull()) {
				// This normally points to an object called "fastCast",
				// but for example in Eco Quest 1 it may also point to an object called "EventHandler" (see bug #5170)
				// Original SCI only checked, if this global was not 0.
				return false;
			}
		}

		signal = readSelectorValue(_s->_segMan, curObject, SELECTOR(signal));
		if (!(signal & kSignalFrozen)) {
			// Call .doit method of that object
			invokeSelector(_s, curObject, SELECTOR(doit), argc, argv, 0);

			// If a game is being loaded, stop processing
			if (_s->abortScriptProcessing != kAbortNone)
				return true; // Stop processing

			// Lookup node again, since the nodetable it was in may have been reallocated.
			// The node might have been deallocated at this point (e.g. LSL2, room 42),
			// in which case the node reference will be null and the loop will stop below.
			// If the node is deleted from kDeleteKey, it won't have a successor node, thus
			// list processing will stop here (which is what SSCI does).
			curNode = _s->_segMan->lookupNode(curAddress, false);
		}

		if (curNode) {
			curAddress = curNode->succ;
			curNode = _s->_segMan->lookupNode(curAddress);
		}
	}
	return true;
}

bool sortHelper(const AnimateEntry &entry1, const AnimateEntry &entry2) {
	if (entry1.y == entry2.y) {
		// if both y and z are the same, use the order we were given originally
		//  this is needed for special cases like iceman room 35
		if (entry1.z == entry2.z)
			return entry1.givenOrderNo < entry2.givenOrderNo;
		else
			return entry1.z < entry2.z;
	}
	return entry1.y < entry2.y;
}
bool hazEnding(std::string const &fullString, std::string const &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	} else {
		return false;
	}
}


Graphics::Surface *loadCelPNG(Common::SeekableReadStream *s) {
	
	Image::PNGDecoder d;

	if (!s)
		return nullptr;
	d.loadStream(*s);
	delete s;

	Graphics::Surface* srf;
	if (d.hasPalette()) {
		srf = d.getSurface()->convertTo(Graphics::PixelFormat::createFormatCLUT8());
	}
	else {
		srf = d.getSurface()->convertTo(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
	}
	return srf;
}

Graphics::Surface *loadCelPNGCLUT(Common::SeekableReadStream *s) {
	Image::PNGDecoder d;

	if (!s)
		return nullptr;
	d.loadStream(*s);
	delete s;
	Graphics::Surface* srf;
	if (d.hasPalette()) {
		srf = d.getSurface()->convertTo(Graphics::PixelFormat::createFormatCLUT8());
	}
	else {
		srf = d.getSurface()->convertTo(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
	}
	return srf;
}

Graphics::Surface* loadCelPNGCLUTOverride(Common::SeekableReadStream* s) {
	Image::PNGDecoder d;
	if (!s)
		return nullptr;
	d.loadStream(*s);
	delete s;
	Graphics::Surface* srf = d.getSurface()->convertTo(Graphics::PixelFormat::createFormatCLUT8(), d.getPalette());
	for (int16 i = 0; i < 256; i++) {
		g_sci->_gfxPalette16->_paletteOverride.colors[i].r = d.getPalette()[i * 3];
		g_sci->_gfxPalette16->_paletteOverride.colors[i].g = d.getPalette()[(i * 3) + 1];
		g_sci->_gfxPalette16->_paletteOverride.colors[i].b = d.getPalette()[(i * 3) + 2];
	}
	g_sci->_gfxPalette16->_sysPalette = g_sci->_gfxPalette16->_paletteOverride;
	//memcpy((void *)g_sci->_gfxPalette16->_paletteOverride, d.getPalette(), sizeof(d.getPalette()));
	//_tehScreen->setPalette(d.getPalette(), 0, 256, true);
	return srf;
}
void GfxAnimate::LoadAllExtraPNG() {
	g_sci->cachedFiles = 0;
	if (g_sci->totalFilesToCache > 0) {

		std::string directory = Common::FSNode(ConfMan.get("extrapath")).getPath().c_str();

		DIR *dir;
		class dirent *ent;
		//class stat st;

		dir = opendir(directory.c_str());
		while ((ent = readdir(dir)) != NULL) {
			const std::string file_name = ent->d_name;
			const std::string full_file_name = directory + "/" + file_name;

			//if (file_name[0] == '.')
			//continue;

			//if (stat(full_file_name.c_str(), &st) == -1)
			//continue;

			//const bool is_directory = (st.st_mode & S_IFDIR) != 0;

			//if (is_directory)
			//continue;

			if (strstr(file_name.c_str(), ".png"))
			{
				if ((strstr(file_name.c_str(), "pic") || strstr(file_name.c_str(), "view")) && !strstr(file_name.c_str(), "_256") && !strstr(file_name.c_str(), "_256RP")) {
					Common::String fn = Common::FSNode(ConfMan.get("extrapath")).getChild(file_name.c_str()).getName();
					Common::SeekableReadStream* file = SearchMan.createReadStreamForMember(fn);
					if (file && !strstr(file_name.c_str(), "_256")) {
						Graphics::Surface* viewpngtmp = loadCelPNG(file);
						if (viewpngtmp) {
							const byte* viewenh = (const byte*)viewpngtmp->getPixels();
							if (viewenh) {
								std::pair<Graphics::Surface*, const byte*> tmp;
								tmp.first = viewpngtmp;
								tmp.second = viewenh;
								viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface*, const byte*> >(file_name.c_str(), tmp));
								debug(fn.c_str());
								debug("LOADED FROM DISC");
								g_sci->cachedFiles++;
								if ((uint)((float)((float)g_sci->cachedFiles / (float)g_sci->totalFilesToCache) * 100.00f) != (uint)g_sci->cachedFilesPercent) {

									char loadstrbuffer[32];
									int retVal, buf_size = 32;
									retVal = snprintf(loadstrbuffer, buf_size, "loading.%u.percent.png", (uint)g_sci->cachedFilesPercent);
									Common::String loadname = loadstrbuffer;
									Common::String fnload = Common::FSNode(ConfMan.get("extrapath")).getChild(loadname).getName();
									Common::SeekableReadStream* fileload = SearchMan.createReadStreamForMember(Common::FSNode(ConfMan.get("extrapath")).getChild(fnload).getName());
									if (fileload) {
										//debug("Found : %s", loadname.c_str());
										Graphics::Surface* viewpngloadtmp = loadCelPNG(fileload);
										g_system->copyRectToScreen(viewpngloadtmp->getPixels(), viewpngloadtmp->w * 4, 0, 0, viewpngloadtmp->w, viewpngloadtmp->h);
										g_system->updateScreen();
									}
									else {
										//debug("Didn't find : %s", loadname.c_str());
									}
									g_sci->cachedFilesPercent = ((float)((float)g_sci->cachedFiles / (float)g_sci->totalFilesToCache) * 100.00f);
								}
							}
						}
					}
					if ((strstr(file_name.c_str(), "pic") || strstr(file_name.c_str(), "view")) && (strstr(file_name.c_str(), "_256") || strstr(file_name.c_str(), "_256RP"))) {
						Graphics::Surface* viewpngtmp = loadCelPNGCLUT(file);
						if (viewpngtmp) {
							const byte* viewenh = (const byte*)viewpngtmp->getPixels();
							if (viewenh) {
								std::pair<Graphics::Surface*, const byte*> tmp;
								tmp.first = viewpngtmp;
								tmp.second = viewenh;
								viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface*, const byte*> >(file_name.c_str(), tmp));
								debug(fn.c_str());
								debug("LOADED FROM DISC");
								g_sci->cachedFiles++;
								if ((uint)((float)((float)g_sci->cachedFiles / (float)g_sci->totalFilesToCache) * 100.00f) != (uint)g_sci->cachedFilesPercent) {

									char loadstrbuffer[32];
									int retVal, buf_size = 32;
									retVal = snprintf(loadstrbuffer, buf_size, "loading.%u.percent.png", (uint)g_sci->cachedFilesPercent);
									Common::String loadname = loadstrbuffer;
									Common::String fnload = Common::FSNode(ConfMan.get("extrapath")).getChild(loadname).getName();
									Common::SeekableReadStream* fileload = SearchMan.createReadStreamForMember(Common::FSNode(ConfMan.get("extrapath")).getChild(fnload).getName());
									if (fileload) {
										//debug("Found : %s", loadname.c_str());
										Graphics::Surface* viewpngloadtmp = loadCelPNG(fileload);
										g_system->copyRectToScreen(viewpngloadtmp->getPixels(), viewpngloadtmp->w * 4, 0, 0, viewpngloadtmp->w, viewpngloadtmp->h);
										g_system->updateScreen();
									}
									else {
										//debug("Didn't find : %s", loadname.c_str());
									}
									g_sci->cachedFilesPercent = ((float)((float)g_sci->cachedFiles / (float)g_sci->totalFilesToCache) * 100.00f);
								}
							}
						}
					}
				}
			}
		}
		closedir(dir);
	}
}
	void GfxAnimate::makeSortedList(List *list) {
	    reg_t curAddress = list->first;
	    Node *curNode = _s->_segMan->lookupNode(curAddress);
	    int16 listNr;
	   
	    // Clear lists
	    _list.clear();
	    _lastCastData.clear();

	    // Fill the list
	    for (listNr = 0; curNode != 0; listNr++) {
		    AnimateEntry listEntry;
		    const reg_t curObject = curNode->value;
		    listEntry.object = curObject;
		    listEntry.castHandle = NULL_REG;

		    // Get data from current object
		    listEntry.givenOrderNo = listNr;
		    listEntry.viewId = readSelectorValue(_s->_segMan, curObject, SELECTOR(view));
		    listEntry.loopNo = readSelectorValue(_s->_segMan, curObject, SELECTOR(loop));
		    listEntry.celNo = readSelectorValue(_s->_segMan, curObject, SELECTOR(cel));
		    listEntry.paletteNo = readSelectorValue(_s->_segMan, curObject, SELECTOR(palette));
		    listEntry.signal = readSelectorValue(_s->_segMan, curObject, SELECTOR(signal));

		    listEntry.y = readSelectorValue(_s->_segMan, curObject, SELECTOR(y));
		    listEntry.x = readSelectorValue(_s->_segMan, curObject, SELECTOR(x));		    
		    listEntry.z = readSelectorValue(_s->_segMan, curObject, SELECTOR(z));
		    listEntry.priority = readSelectorValue(_s->_segMan, curObject, SELECTOR(priority));
		    if (getSciVersion() >= SCI_VERSION_1_1) {
			    // Cel scaling
			    listEntry.scaleSignal = readSelectorValue(_s->_segMan, curObject, SELECTOR(scaleSignal));
			    if (listEntry.scaleSignal & kScaleSignalDoScaling) {
				    listEntry.scaleX = readSelectorValue(_s->_segMan, curObject, SELECTOR(scaleX));
				    listEntry.scaleY = readSelectorValue(_s->_segMan, curObject, SELECTOR(scaleY));
			    } else {
				    listEntry.scaleX = 128;
				    listEntry.scaleY = 128;
			    }
		    } else {
			    listEntry.scaleSignal = 0;
			    listEntry.scaleX = 128;
			    listEntry.scaleY = 128;
		    }
		    listEntry.showBitsFlag = false;
		    //
			{
			    char viewstrbuffer[32];
			    int retVal, buf_size = 32;
			    retVal = snprintf(viewstrbuffer, buf_size, "view.%u.%u.%u", listEntry.viewId, listEntry.loopNo, listEntry.celNo);
			    Common::String fn = viewstrbuffer;

			    if (g_sci->stereoscopic && g_sci->stereoRightEye) {
				    if (fileIsInExtraDIR((fn + ".reye.png").c_str()))
					    fn += ".reye";
			    }

			    if (videoCutsceneEnd == fn.c_str()) {
				    playingVideoCutscenes = false;
				    wasPlayingVideoCutscenes = true;
				    videoCutsceneEnd = "-undefined-";
				    videoCutsceneStart = "-undefined-";
				    g_system->getMixer()->muteSoundType(Audio::Mixer::kMusicSoundType, false);
				    g_system->getMixer()->muteSoundType(Audio::Mixer::kSFXSoundType, false);
				    g_system->getMixer()->muteSoundType(Audio::Mixer::kSpeechSoundType, false);
				    Common::String dbg = "Cutscene ENDED on : " + fn;
				    debug(dbg.c_str());
			    }
			    if (!extraDIRList.empty() && !wasPlayingVideoCutscenes) {
				    if (fileIsInExtraDIR((fn + ".cts").c_str())) {
					    Common::String cfgfileName = fn + ".cts";
					    debug(cfgfileName.c_str());
					    Common::SeekableReadStream *cfg = SearchMan.createReadStreamForMember(cfgfileName);
					    if (cfg) {
						    Common::String line, texttmp;
						    cutscene_mute_midi = false;
						    while (!cfg->eos()) {
							    texttmp = cfg->readLine();
							    if (texttmp.firstChar() != '#') {
								    if (texttmp.contains("mute_midi")) {
									    cutscene_mute_midi = true;
								    } else {
									    videoCutsceneEnd = texttmp.c_str();
								    }
							    }
						    }
						    videoCutsceneStart = viewstrbuffer;

						    g_sci->oggBackground = fn + ".ogg";

						    g_sci->_theoraDecoderCutscenes = new Video::TheoraDecoder();

						    g_sci->_theoraDecoderCutscenes->loadFile(fn + ".ogg");
						    g_sci->_theoraDecoderCutscenes->start();
						    int16 frameTime = g_sci->_theoraDecoderCutscenes->getTimeToNextFrame();

						    playingVideoCutscenes = true;
						    wasPlayingVideoCutscenes = true;
						    g_system->getMixer()->muteSoundType(Audio::Mixer::kMusicSoundType, true);
						    g_system->getMixer()->muteSoundType(Audio::Mixer::kSFXSoundType, true);
						    g_system->getMixer()->muteSoundType(Audio::Mixer::kSpeechSoundType, true);

						    if (cutscene_mute_midi) {
							    if (midiMusic != NULL)
								    midiMusic->setMasterVolume(0);
						    }
						    Common::String dbg = "Cutscene STARTED on : " + fn;
						    debug(dbg.c_str());
						    dbg = "Cutscene set to end on : ";
						    dbg += videoCutsceneEnd.c_str();
						    debug(dbg.c_str());
					    }
				    } else {
					    debug(10, ("NO " + fn + ".cts").c_str());
				    }
			    }
			    if (g_sci->enhanced_gfx_enabled) {
			    
			    bool preloaded = false;
			    listEntry.viewpng = NULL;
			    listEntry.viewenh = NULL;
			    listEntry.viewEnhanced = false;
				debug("Attempting to load %s.png, %s_256.png, or %s_256RP.png", fn.c_str(), fn.c_str(), fn.c_str());
			    if (viewsMap.size() > 0)
				    for (viewsMapit = viewsMap.begin();
				         viewsMapit != viewsMap.end(); ++viewsMapit) {

					    if (strcmp(viewsMapit->first.c_str(), (fn + ".png").c_str()) == 0) {

						    //debug(viewsMapit->first.c_str());
						    std::pair<Graphics::Surface *, const byte *> tmp = viewsMapit->second;
						    listEntry.viewpng = tmp.first;
						    //debug("RELOADED FROM RAM");
						    listEntry.viewenh = tmp.second;
						    if (listEntry.viewenh) {
							    preloaded = true;
							    listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
							    listEntry.viewEnhanced = true;
							    listEntry.enhancedIs256 = (listEntry.viewpng->format != Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
						    }
					    }
						if (strcmp(viewsMapit->first.c_str(), (fn + "_256.png").c_str()) == 0) {

							//debug(viewsMapit->first.c_str());
							std::pair<Graphics::Surface*, const byte*> tmp = viewsMapit->second;
							listEntry.viewpng = tmp.first;
							//debug("RELOADED FROM RAM");
							listEntry.viewenh = tmp.second;
							if (listEntry.viewenh) {
								preloaded = true;
								listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
								listEntry.viewEnhanced = true;
								listEntry.enhancedIs256 = true;
							}
						}
						if (strcmp(viewsMapit->first.c_str(), (fn + "_256RP.png").c_str()) == 0) {

							//debug(viewsMapit->first.c_str());
							std::pair<Graphics::Surface*, const byte*> tmp = viewsMapit->second;
							listEntry.viewpng = tmp.first;
							//debug("RELOADED FROM RAM");
							listEntry.viewenh = tmp.second;
							if (listEntry.viewenh) {
								preloaded = true;
								listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
								listEntry.viewEnhanced = true;
								listEntry.enhancedIs256 = true;
							}
						}
				    }
			    if (!preloaded) {

				    if (!extraDIRList.empty()) {
						if (fileIsInExtraDIR((fn + ".png").c_str())) {
							if (!listEntry.viewEnhanced) {
								Common::String fileName = fn + ".png";
								Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
								if (!file) {
									//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
								} else {
									////debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
									Graphics::Surface *viewpngtmp = loadCelPNG(file);
									listEntry.viewpng = viewpngtmp;
									if (listEntry.viewpng) {
										const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
										listEntry.viewenh = viewenhtmp;
										if (listEntry.viewenh) {
											listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
											listEntry.viewEnhanced = true;
											listEntry.enhancedIs256 = (listEntry.viewpng->format != Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
											std::pair<Graphics::Surface *, const byte *> tmp;
											tmp.first = viewpngtmp;
											tmp.second = viewenhtmp;
											viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + ".png").c_str(), tmp));
											//debug(fn.c_str());
											//debug("LOADED FROM DISC");
										}
									}
								}
							}
						} else if (fileIsInExtraDIR((fn + "_256.png").c_str())) {
							if (!listEntry.viewEnhanced) {
								Common::String fileName = fn + "_256.png";
								Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
								if (!file) {
									//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
								} else {
									////debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
									Graphics::Surface *viewpngtmp = loadCelPNGCLUT(file);
									listEntry.viewpng = viewpngtmp;
									if (listEntry.viewpng) {
										const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
										listEntry.viewenh = viewenhtmp;
										if (listEntry.viewenh) {
											listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
											listEntry.viewEnhanced = true;
											listEntry.enhancedIs256 = true;
											std::pair<Graphics::Surface *, const byte *> tmp;
											tmp.first = viewpngtmp;
											tmp.second = viewenhtmp;
											viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + "_256.png").c_str(), tmp));
											//debug(fn.c_str());
											//debug("LOADED FROM DISC");
										}
									}
								}
							}
						} else if (fileIsInExtraDIR((fn + "_256RP.png").c_str())) {
							if (!listEntry.viewEnhanced) {
								Common::String fileName = fn + "_256RP.png";
								Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
								if (!file) {
									//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
								} else {
									//debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
									Graphics::Surface *viewpngtmp = loadCelPNGCLUTOverride(file);
									listEntry.viewpng = viewpngtmp;
									if (listEntry.viewpng) {
										const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
										listEntry.viewenh = viewenhtmp;
										if (listEntry.viewenh) {
											listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
											listEntry.viewEnhanced = true;
											listEntry.enhancedIs256 = true;
											std::pair<Graphics::Surface *, const byte *> tmp;
											tmp.first = viewpngtmp;
											tmp.second = viewenhtmp;
											viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + "_256RP.png").c_str(), tmp));
											//debug(fn.c_str());
											//debug("LOADED FROM DISC");
										}
									}
								}
							}
						}
					}
				    }
			    }
		    }

	    _list.push_back(listEntry);

	    curAddress = curNode->succ;
	    curNode = _s->_segMan->lookupNode(curAddress);
    }

	// Possible TODO: As noted in the comment in sortHelper we actually
	// require a stable sorting algorithm here. Since Common::sort is not stable
	// at the time of writing this comment, we work around that in our ordering
	// comparator. If that changes in the future or we want to use some
	// stable sorting algorithm here, we should change that.
	// In that case we should test such changes intensively. A good place to test stable sort
	// is iceman, cupboard within the submarine. If sort isn't stable, the cupboard will be
	// half-open, half-closed. Of course that's just one of many special cases.

	// Now sort the list according y and z (descending)
	Common::sort(_list.begin(), _list.end(), sortHelper);
	_newList.clear();
}

void GfxAnimate::fill(byte &old_picNotValid) {
	GfxView *view = nullptr;
	AnimateList::iterator it;
	const AnimateList::iterator end = _list.end();

	for (it = _list.begin(); it != end; ++it) {
		// Get the corresponding view
		view = _cache->getView(it->viewId);

		adjustInvalidCels(view, it);
		processViewScaling(view, it);
		setNsRect(view, it);

		//warning("%s view %d, loop %d, cel %d, signal %x", _s->_segMan->getObjectName(curObject), it->viewId, it->loopNo, it->celNo, it->signal);

		// Calculate current priority according to y-coordinate
		if (!(it->signal & kSignalFixedPriority)) {
			it->priority = _ports->kernelCoordinateToPriority(it->y);
			writeSelectorValue(_s->_segMan, it->object, SELECTOR(priority), it->priority);
		}

		if (it->signal & kSignalNoUpdate) {
			if ((it->signal & (kSignalForceUpdate | kSignalViewUpdated))
				||   (it->signal & kSignalHidden  && !(it->signal & kSignalRemoveView))
				|| (!(it->signal & kSignalHidden) &&   it->signal & kSignalRemoveView)
				||   (it->signal & kSignalAlwaysUpdate))
				old_picNotValid++;
			it->signal &= ~kSignalStopUpdate;
		} else {
			if ((it->signal & kSignalStopUpdate) || (it->signal & kSignalAlwaysUpdate))
				old_picNotValid++;
			it->signal &= ~kSignalForceUpdate;
		}
	}
}

void GfxAnimate::adjustInvalidCels(GfxView *view, AnimateList::iterator it) {
	// adjust loop and cel, if any of those is invalid
	//  this seems to be completely crazy code
	//  sierra sci checked signed int16 to be above or equal the counts and reseted to 0 in those cases
	//  later during view processing those are compared unsigned again and then set to maximum count - 1
	//  Games rely on this behavior. For example laura bow 1 has a knight standing around in room 37
	//   which has cel set to 3. This cel does not exist and the actual knight is 0
	//   In kq5 on the other hand during the intro, when the trunk is opened, cel is set to some real
	//   high number, which is negative when considered signed. This actually requires to get fixed to
	//   maximum cel, otherwise the trunk would be closed.
	int16 viewLoopCount = view->getLoopCount();
	if (it->loopNo >= viewLoopCount) {
		it->loopNo = 0;
		writeSelectorValue(_s->_segMan, it->object, SELECTOR(loop), it->loopNo);
	} else if (it->loopNo < 0) {
		it->loopNo = viewLoopCount - 1;
		// not setting selector is right, sierra sci didn't do it during view processing as well
	}
	int16 viewCelCount = view->getCelCount(it->loopNo);
	if (it->celNo >= viewCelCount) {
		it->celNo = 0;
		writeSelectorValue(_s->_segMan, it->object, SELECTOR(cel), it->celNo);
	} else if (it->celNo < 0) {
		it->celNo = viewCelCount - 1;
	}
}

void GfxAnimate::processViewScaling(GfxView *view, AnimateList::iterator it) {
	if (!view->isScaleable()) {
		// Laura Bow 2 (especially floppy) depends on this, some views are not supposed to be scaleable
		//  this "feature" was removed in later versions of SCI1.1
		it->scaleSignal = 0;
		it->scaleY = it->scaleX = 128;
	} else {
		// Process global scaling, if needed
		if (it->scaleSignal & kScaleSignalDoScaling) {
			if (it->scaleSignal & kScaleSignalGlobalScaling) {
				applyGlobalScaling(it, view);
			}
		}
	}
}

void GfxAnimate::applyGlobalScaling(AnimateList::iterator entry, GfxView *view) {
	// Global scaling uses global var 2 and some other stuff to calculate scaleX/scaleY
	int16 maxScale = readSelectorValue(_s->_segMan, entry->object, SELECTOR(maxScale));
	int16 celHeight = view->getHeight(entry->loopNo, entry->celNo);
	int16 maxCelHeight = (maxScale * celHeight) >> 7;
	reg_t globalVar2 = _s->variables[VAR_GLOBAL][kGlobalVarCurrentRoom]; // current room object
	int16 vanishingY = readSelectorValue(_s->_segMan, globalVar2, SELECTOR(vanishingY));

	int16 fixedPortY = _ports->getPort()->rect.bottom - vanishingY;
	int16 fixedEntryY = entry->y - vanishingY;
	if (!fixedEntryY)
		fixedEntryY = 1;

	if ((celHeight == 0) || (fixedPortY == 0))
		error("global scaling panic");

	entry->scaleY = ( maxCelHeight * fixedEntryY ) / fixedPortY;
	entry->scaleY = (entry->scaleY * 128) / celHeight;

	entry->scaleX = entry->scaleY;

	// and set objects scale selectors
	writeSelectorValue(_s->_segMan, entry->object, SELECTOR(scaleX), entry->scaleX);
	writeSelectorValue(_s->_segMan, entry->object, SELECTOR(scaleY), entry->scaleY);
}

void GfxAnimate::setNsRect(GfxView *view, AnimateList::iterator it) {
	bool shouldSetNsRect = true;

	// Create rect according to coordinates and given cel
	if (it->scaleSignal & kScaleSignalDoScaling) {
		if ((!g_sci->enhanced_DEPTH && !g_sci->depth_rendering) || (!g_sci->enhanced_DEPTH && !g_sci->stereo_pair_rendering)) {
			if (!it->viewEnhanced) {
				view->getCelScaledRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->scaleX, it->scaleY, it->celRect);
			} else {
				view->getCelScaledRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->scaleX, it->scaleY, it->celRect);
			}
		} else {

			

				if (g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X && (((int16)g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier)) > 16 && (((int16)g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier)) < g_sci->_gfxScreen->getScriptWidth() - 16) {

					if (!it->viewEnhanced) {
						view->getCelScaledRect(it->loopNo, it->celNo, (((int16)g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier)), ((int16)g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_Y, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier), it->z, it->scaleX, it->scaleY, it->celRect);
					} else {
						view->getCelScaledRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, (((int16)g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier)), ((int16)g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_Y, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier), it->z, it->scaleX, it->scaleY, it->celRect);
					}
					it->celRect.clip(_ports->_curPort->rect);
					it->bitsRect.clip(_ports->_curPort->rect);
				} else {
					if (!it->viewEnhanced) {
						view->getCelScaledRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->scaleX, it->scaleY, it->celRect);
					} else {
						view->getCelScaledRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->scaleX, it->scaleY, it->celRect);
					}
					it->celRect.clip(_ports->_curPort->rect);
					it->bitsRect.clip(_ports->_curPort->rect);
				}
			
		}
		// when being scaled, only set nsRect, if object will get drawn
		if ((it->signal & kSignalHidden) && !(it->signal & kSignalAlwaysUpdate))
			shouldSetNsRect = false;
	} else {
		//  This special handling is not included in the other SCI1.1 interpreters and MUST NOT be
		//  checked in those cases, otherwise we will break games (e.g. EcoQuest 2, room 200)
		if ((g_sci->getGameId() == GID_HOYLE4) && (it->scaleSignal & kScaleSignalHoyle4SpecialHandling)) {
			it->celRect = g_sci->_gfxCompare->getNSRect(it->object);
			view->getCelSpecialHoyle4Rect(it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
			shouldSetNsRect = false;
		} else {
			if (!g_sci->enhanced_DEPTH || !g_sci->depth_rendering) {
				if (!it->viewEnhanced) {
					view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
					view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
				} else {
					view->getCelRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
					view->getCelRectEnhancedBits(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
					it->bitsRect.clip(_ports->_curPort->rect);
				}
			} else {
				if (it->signal & kSignalNoUpdate) {
					if (!it->viewEnhanced) {
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
					} else {
						view->getCelRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
						view->getCelRectEnhancedBits(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
						it->bitsRect.clip(_ports->_curPort->rect);
					}
				} else {
				
				if (it->x > 0 && it->x < g_sci->_gfxScreen->_scriptWidth && it->y > 0 && it->y < g_sci->_gfxScreen->_scriptHeight) {

					if (!it->viewEnhanced) {
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
						view->getCelRect(it->loopNo, it->celNo, (int16)clip((int)(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, (it->y - (it->celRect.height() / 2)) * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier), 0, (int)(g_sci->_gfxScreen->_scriptWidth)), (int16)clip(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_Y, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier, 0, (int)(g_sci->_gfxScreen->_scriptHeight)), it->z, it->celRect);
						view->getCelRect(it->loopNo, it->celNo, (int16)clip((int)(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, (it->y - (it->celRect.height() / 2)) * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier), 0, (int)(g_sci->_gfxScreen->_scriptWidth)), (int16)clip(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_Y, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier, 0, (int)(g_sci->_gfxScreen->_scriptHeight)), it->z, it->bitsRect);
					} else {
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
						view->getCelRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, (int16)clip((int)(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, (it->y - (it->celRect.height() / 2)) * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier), 0, (int)(g_sci->_gfxScreen->_scriptWidth)), (int16)clip(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_Y, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier, 0, (int)(g_sci->_gfxScreen->_scriptHeight)), it->z, it->celRect);
						view->getCelRectEnhancedBits(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, (int16)clip((int)(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_X, it->x * g_sci->_enhancementMultiplier, (it->y - (it->celRect.height() / 2)) * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier), 0, (int)(g_sci->_gfxScreen->_scriptWidth)), (int16)clip(g_sci->_gfxScreen->getDepthShift(g_sci->_gfxScreen->_displayScreenDEPTH_SHIFT_Y, it->x * g_sci->_enhancementMultiplier, it->y * g_sci->_enhancementMultiplier) / g_sci->_enhancementMultiplier, 0, (int)(g_sci->_gfxScreen->_scriptHeight)), it->z, it->bitsRect);
						
						it->bitsRect.clip(_ports->_curPort->rect);
					}
					
				} else {
					if (!it->viewEnhanced) {
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
						view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
					} else {
						view->getCelRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
						view->getCelRectEnhancedBits(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
						
						it->bitsRect.clip(_ports->_curPort->rect);
					}
				}
				}
			}
		}
	}

	if (shouldSetNsRect) {
		if (_screen->_upscaledHires == GFX_SCREEN_UPSCALED_640x400) {
			g_sci->_gfxCompare->setNSRect(it->object, it->bitsRect);
		} else {
			g_sci->_gfxCompare->setNSRect(it->object, it->celRect);
		}
	}
}

void GfxAnimate::update() {
	reg_t bitsHandle;
	Common::Rect rect;
	AnimateList::iterator it;
	const AnimateList::iterator end = _list.end();

	// Remove all no-update cels, if requested
	if (!g_sci->enhanced_DEPTH || !g_sci->depth_rendering) {
		if (!g_sci->backgroundIsVideo && !g_sci->play_enhanced_BG_anim) {
			for (it = _list.reverse_begin(); it != end; --it) {
				if (it->signal & kSignalNoUpdate) {
					if (!(it->signal & kSignalRemoveView)) {
						bitsHandle = readSelector(_s->_segMan, it->object, SELECTOR(underBits));
						if (_screen->_picNotValid != 1) {
							_paint16->bitsRestore(bitsHandle);
							it->showBitsFlag = true;
						} else {
							_paint16->bitsFree(bitsHandle);
						}
						writeSelectorValue(_s->_segMan, it->object, SELECTOR(underBits), 0);
					}
					it->signal &= ~kSignalForceUpdate;
					if (it->signal & kSignalViewUpdated)
						it->signal &= ~(kSignalViewUpdated | kSignalNoUpdate);
				} else if (it->signal & kSignalStopUpdate) {
					it->signal &= ~kSignalStopUpdate;
					it->signal |= kSignalNoUpdate;
				}
			}
		}
	}
	
	// Draw always-update cels
	for (it = _list.begin(); it != end; ++it) {

		if (it->signal & kSignalAlwaysUpdate) {

			// draw corresponding cel
			_paint16->drawCel(it->viewpng, it->viewenh, it->pixelsLength, it->viewEnhanced, it->enhancedIs256, it->viewId, it->loopNo, it->celNo, 0, it->celRect, it->priority, it->paletteNo, it->scaleX, it->scaleY);
			it->showBitsFlag = true;
			
				

				
			
			it->signal &= ~(kSignalStopUpdate | kSignalViewUpdated | kSignalNoUpdate | kSignalForceUpdate);
			if (!(it->signal & kSignalIgnoreActor)) {
				
					rect = it->celRect;
				
				rect.top = CLIP<int16>(_ports->kernelPriorityToCoordinate(it->priority) - 1, rect.top, rect.bottom - 1);
				
				_paint16->fillRect(rect, GFX_SCREEN_MASK_CONTROL, 0, 0, 15);
			}
		}
	}

	// Saving background for all NoUpdate-cels
	for (it = _list.begin(); it != end; ++it) {
		if (it->signal & kSignalNoUpdate) {
			if (it->signal & kSignalHidden) {
				it->signal |= kSignalRemoveView;
			} else {
				it->signal &= ~kSignalRemoveView;
				if (_screen->_upscaledHires == GFX_SCREEN_UPSCALED_640x400) {
					if (it->signal & kSignalIgnoreActor)
						bitsHandle = _paint16->bitsSave(it->bitsRect, GFX_SCREEN_MASK_VISUAL | GFX_SCREEN_MASK_PRIORITY);
					else
						bitsHandle = _paint16->bitsSave(it->bitsRect, GFX_SCREEN_MASK_ALL);
					writeSelector(_s->_segMan, it->object, SELECTOR(underBits), bitsHandle);
				} else {
					if (it->signal & kSignalIgnoreActor)
						bitsHandle = _paint16->bitsSave(it->celRect, GFX_SCREEN_MASK_VISUAL | GFX_SCREEN_MASK_PRIORITY);
					else
						bitsHandle = _paint16->bitsSave(it->celRect, GFX_SCREEN_MASK_ALL);
					writeSelector(_s->_segMan, it->object, SELECTOR(underBits), bitsHandle);
				}
			}
		}
	}

	// Draw NoUpdate cels
	for (it = _list.begin(); it != end; ++it) {
		if (it->signal & kSignalNoUpdate && !(it->signal & kSignalHidden)) {
			// draw corresponding cel
			
			_paint16->drawCel(it->viewpng, it->viewenh, it->pixelsLength, it->viewEnhanced, it->enhancedIs256, it->viewId, it->loopNo, it->celNo, 0, it->celRect, it->priority, it->paletteNo, it->scaleX, it->scaleY);

			it->showBitsFlag = true;

			if (!(it->signal & kSignalIgnoreActor)) {
				rect = it->celRect;
				rect.top = CLIP<int16>(_ports->kernelPriorityToCoordinate(it->priority) - 1, rect.top, rect.bottom - 1);
				_paint16->fillRect(rect, GFX_SCREEN_MASK_CONTROL, 0, 0, 15);
			}
		}
	}
	

}

void GfxAnimate::drawCels() {

	int16 calcAvgPosX = 0;
	int16 calcAvgPosY = 0;
	int16 numberOfViews = 0;
	AnimateList::iterator itxy;
	const AnimateList::iterator endxy = _list.end();
	for (itxy = _list.begin(); itxy != endxy; ++itxy) {
		if (!(itxy->signal & kSignalNoUpdate) && !(itxy->signal & kSignalHidden) && !(itxy->signal & kSignalFrozen)) {
			calcAvgPosX += itxy->x;
			calcAvgPosY += itxy->y;

			numberOfViews++;
		}
	}

	if (numberOfViews > 0) {
		Common::Point p;
		if (g_sci->avgViewPos.size() >= 29) {
			for (int n = 29; n < g_sci->avgViewPos.size(); n++) {
				g_sci->avgViewPos.erase(g_sci->avgViewPos.begin());
			}
		}
		p.x = (int)clip(((float)((float)calcAvgPosX / (float)numberOfViews)), 0, g_sci->_gfxScreen->_scriptWidth);
		p.y = (int)clip(((float)((float)calcAvgPosY / (float)numberOfViews)), 0, g_sci->_gfxScreen->_scriptHeight);
		
		
			for (int n = g_sci->avgViewPos.size(); n < 30; n++) {
				g_sci->avgViewPos.push_back(p);
			}
		
	}
	if (g_sci->avgViewPos.size() > 0) {

		for (std::list<Common::Point>::iterator itavp = g_sci->avgViewPos.begin(); itavp != g_sci->avgViewPos.end(); ++itavp) {
			g_sci->viewLookPos.x += itavp->x;
			g_sci->viewLookPos.y += itavp->y;
		}
		g_sci->viewLookPos.x /= g_sci->avgViewPos.size();
		g_sci->viewLookPos.y /= g_sci->avgViewPos.size();
		g_sci->viewLookPos.x *= g_sci->_enhancementMultiplier;
		g_sci->viewLookPos.y *= g_sci->_enhancementMultiplier;
	}
	if (g_sci->depth_rendering)
	if (g_sci->enhanced_DEPTH) {
		g_sci->_gfxScreen->renderFrameDepthFirst((g_sci->mouseLookPos.x - (g_sci->_gfxScreen->_displayWidth / 2)) + ((g_sci->viewLookPos.x - (g_sci->_gfxScreen->_displayWidth / 2))*4.000f), (g_sci->mouseLookPos.y - (g_sci->_gfxScreen->_displayHeight / 2)) + ((g_sci->viewLookPos.y - (g_sci->_gfxScreen->_displayHeight / 2))*4.000f));
	}
	
	reg_t bitsHandle;
	AnimateList::iterator it;
	const AnimateList::iterator end = _list.end();
	_lastCastData.clear();
	if (!playingVideoCutscenes) {
		EngineState *s = g_sci->getEngineState();
		if (!g_sci->backgroundIsVideo) {
			//every2ndFrame = !every2ndFrame;
			//if (every2ndFrame)
			{
				if (g_system->getMillis() - s->_screenUpdateTime >= (1000 / 30) * 2) {

					int16 frameDrop = (int)((g_system->getMillis() - s->_screenUpdateTime) / (1000 / 30));
					if (frameDrop > 0) {
						g_sci->enhanced_bg_frame += frameDrop;
					}
				}
				g_sci->enhanced_bg_frame++;
				//reg_t screenBits = g_sci->_gfxPaint16->bitsSave(g_sci->_gfxPorts->_picWind->rect, 1 | 2);
				//debug("ANIMATING PIC BACKGROUND!");
				//g_sci->dontUpdate = true;
				

				if (g_sci->play_enhanced_BG_anim) {
					if (g_sci->prevPictureId != NULL) {
						//if (g_sci->_gfxPorts->_curPort->top == 10)
						{
							//g_sci->_gfxPorts->beginUpdate(g_sci->_gfxPorts->_picWind);
							g_sci->_gfxPaint16->drawPicture(g_sci->prevPictureId, g_sci->prevMirroredFlag, true, (GuiResourceId)g_sci->prevPaletteId);
							//Common::Rect _animDrawArea = Common::Rect(0, 0, g_sci->_gfxScreen->getScriptWidth(), g_sci->_gfxScreen->getScriptHeight());
							//g_sci->_gfxAnimate->reAnimate(_animDrawArea);
							//g_sci->_gfxPaint16->bitsRestore(g_sci->menuSaveBits);
							//g_sci->_gfxPorts->endUpdate(g_sci->_gfxPorts->_picWind);
						}
					}
					//Common::Rect _animDrawArea = g_sci->_gfxPorts->getPort()->rect;
					//_animDrawArea.bottom = g_sci->_gfxPorts->_picWind->rect.top;
					//_animDrawArea.top = 0;
					//g_sci->_gfxPaint16->fillRect(_animDrawArea, 1, g_sci->_gfxScreen->getColorWhite());
					//g_sci->bitsHandleMenu = g_sci->_gfxPaint16->bitsSave(g_sci->_gfxPorts->_curPort->rect, 1 | 2);
					//g_sci->sleep(10);
					//if (getSciVersion() >= SCI_VERSION_1_EARLY)
					//g_sci->_gfxScreen->_picNotValid = 1;
					//g_sci->_gfxAnimate->reAnimate(g_sci->_gfxPorts->_currentViewPort);
					
					//g_sci->_gfxPaint16->bitsShow(g_sci->_gfxPorts->getPort()->rect);

					//	g_sci->_gfxPaint16->bitsRestore(screenBits);
					//g_sci->_gfxPaint16->bitsRestore(g_sci->bitsHandleMenu);
					//g_sci->dontUpdate = true;
				}
			}
			//reAnimate(_ports->_curPort->rect);
		} else {
			if (!playingVideoCutscenes) {

				// Throttle the checking of shouldQuit() to 60fps as well, since
				// Engine::shouldQuit() invokes 2 virtual functions
				// (EventManager::shouldQuit() and EventManager::shouldReturnToLauncher()),
				// which is very expensive to invoke constantly without any
				// throttling at all.
				if (g_system->getMillis() - s->_screenUpdateTime >= g_sci->_theoraDecoder->getTimeToNextFrame() * 2) {
					if (g_sci->_theoraDecoder->getTimeToNextFrame() != 0) {
						int16 frameDrop = (int)((g_system->getMillis() - s->_screenUpdateTime) / g_sci->_theoraDecoder->getTimeToNextFrame());
						if (frameDrop > 0) {
							g_sci->_theoraDecoder->seekToFrame(g_sci->_theoraDecoder->getCurFrame() + frameDrop);
						}
					}
				}

				if (g_sci->prevPictureId != NULL) {
					//if (g_sci->_gfxPorts->_curPort->top == 10)
					{
						if (g_system->getMillis() - s->_screenUpdateTime >= g_sci->_theoraDecoder->getTimeToNextFrame()) {

							s->_screenUpdateTime = g_system->getMillis();
							if (g_sci->_theoraDecoder->getCurFrame() == -1) {
								//g_sci->_theoraDecoder->decodeNextFrame();
							} else {
								//g_sci->_theoraSurface = g_sci->_theoraDecoder->decodeNextFrame();

								//if (g_sci->_theoraSurface != nullptr)
								{
									if (g_sci->prevPictureId != NULL) {
										//g_sci->_gfxPorts->beginUpdate(g_sci->_gfxPorts->_picWind);
										g_sci->_gfxPaint16->drawPicture(g_sci->prevPictureId, g_sci->prevMirroredFlag, true, (GuiResourceId)g_sci->prevPaletteId);
										//Common::Rect _animDrawArea = Common::Rect(0, 0, g_sci->_gfxScreen->getScriptWidth(), g_sci->_gfxScreen->getScriptHeight());
										//g_sci->_gfxAnimate->reAnimate(_animDrawArea);
										//g_sci->_gfxPaint16->bitsRestore(g_sci->menuSaveBits);
										//g_sci->_gfxPorts->endUpdate(g_sci->_gfxPorts->_picWind);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	for (it = _list.begin(); it != end; ++it) {
		

		
		if (!(it->signal & (kSignalNoUpdate | kSignalHidden | kSignalAlwaysUpdate))) {
			// Save background
			//_paint16->frameRect(it->bitsRect);
			if (_screen->_upscaledHires == GFX_SCREEN_UPSCALED_640x400 || _screen->_upscaledHires == GFX_SCREEN_UPSCALED_320x200_X_EGA) {
				// Save background
				//_paint16->frameRect(it->bitsRect);
				if (it->bitsRect.left > it->bitsRect.right) {
					int l = it->bitsRect.left;
					it->bitsRect.left = it->bitsRect.right;
					it->bitsRect.right = l;
				}
				if (it->bitsRect.top > it->bitsRect.bottom) {
					int t = it->bitsRect.top;
					it->bitsRect.top = it->bitsRect.bottom;
					it->bitsRect.bottom = t;
				}
				bitsHandle = _paint16->bitsSave(it->bitsRect, GFX_SCREEN_MASK_ALL);
				writeSelector(_s->_segMan, it->object, SELECTOR(underBits), bitsHandle);
				// draw corresponding cel
			} else {
				// Save background
				//_paint16->frameRect(it->bitsRect);
				if (it->celRect.left > it->celRect.right) {
					int l = it->celRect.left;
					it->celRect.left = it->celRect.right;
					it->celRect.right = l;
				}
				if (it->celRect.top > it->celRect.bottom) {
					int t = it->celRect.top;
					it->celRect.top = it->celRect.bottom;
					it->celRect.bottom = t;
				}
				bitsHandle = _paint16->bitsSave(it->celRect, GFX_SCREEN_MASK_ALL);
				writeSelector(_s->_segMan, it->object, SELECTOR(underBits), bitsHandle);
			}
			if (it->signal & kSignalNoUpdate) {
				// Save background
				//_paint16->frameRect(it->bitsRect);
				if (_screen->_upscaledHires == GFX_SCREEN_UPSCALED_640x400 || _screen->_upscaledHires == GFX_SCREEN_UPSCALED_320x200_X_EGA) {
					// Save background
					//_paint16->frameRect(it->bitsRect);
					if (it->bitsRect.left > it->bitsRect.right) {
						int l = it->bitsRect.left;
						it->bitsRect.left = it->bitsRect.right;
						it->bitsRect.right = l;
					}
					if (it->bitsRect.top > it->bitsRect.bottom) {
						int t = it->bitsRect.top;
						it->bitsRect.top = it->bitsRect.bottom;
						it->bitsRect.bottom = t;
					}
					bitsHandle = _paint16->bitsSave(it->bitsRect, GFX_SCREEN_MASK_ALL);
					writeSelector(_s->_segMan, it->object, SELECTOR(underBits), bitsHandle);
					// draw corresponding cel
				} else {
					// Save background
					//_paint16->frameRect(it->bitsRect);
					if (it->celRect.left > it->celRect.right) {
						int l = it->celRect.left;
						it->celRect.left = it->celRect.right;
						it->celRect.right = l;
					}
					if (it->celRect.top > it->celRect.bottom) {
						int t = it->celRect.top;
						it->celRect.top = it->celRect.bottom;
						it->celRect.bottom = t;
					}
					bitsHandle = _paint16->bitsSave(it->celRect, GFX_SCREEN_MASK_ALL);
					writeSelector(_s->_segMan, it->object, SELECTOR(underBits), bitsHandle);
				}
			}

				_paint16->drawCel(it->viewpng, it->viewenh, it->pixelsLength, it->viewEnhanced, it->enhancedIs256, it->viewId, it->loopNo, it->celNo, 0, it->celRect, it->priority, it->paletteNo, it->scaleX, it->scaleY, it->scaleSignal);

			it->showBitsFlag = true;

			if (it->signal & kSignalRemoveView)
				it->signal &= ~kSignalRemoveView;

			// Remember that entry in lastCast
			_lastCastData.push_back(*it);
		}
	}
}

void GfxAnimate::updateScreen(byte oldPicNotValid) {

	if (!playingVideoCutscenes) {
		AnimateList::iterator it;
		const AnimateList::iterator end = _list.end();
		Common::Rect lsRect;
		Common::Rect workerRect;
		if (workerRect.left > workerRect.right) {
			int l = workerRect.left;
			workerRect.left = workerRect.right;
			workerRect.right = l;
		}
		if (workerRect.top > workerRect.bottom) {
			int t = workerRect.top;
			workerRect.top = workerRect.bottom;
			workerRect.bottom = t;
		}
		for (it = _list.begin(); it != end; ++it) {
			if (it->showBitsFlag || !(it->signal & (kSignalRemoveView | kSignalNoUpdate) ||
			                          (!(it->signal & kSignalRemoveView) && (it->signal & kSignalNoUpdate) && oldPicNotValid))) {
				lsRect.left = readSelectorValue(_s->_segMan, it->object, SELECTOR(lsLeft)) / 2;
				lsRect.top = readSelectorValue(_s->_segMan, it->object, SELECTOR(lsTop)) / 2;
				lsRect.right = readSelectorValue(_s->_segMan, it->object, SELECTOR(lsRight)) / 2;
				lsRect.bottom = readSelectorValue(_s->_segMan, it->object, SELECTOR(lsBottom)) / 2;
				if (it->celRect.left > it->celRect.right) {
					int l = it->celRect.left;
					it->celRect.left = it->celRect.right;
					it->celRect.right = l;
				}
				if (it->celRect.top > it->celRect.bottom) {
					int t = it->celRect.top;
					it->celRect.top = it->celRect.bottom;
					it->celRect.bottom = t;
				}
				if (it->bitsRect.left > it->bitsRect.right) {
					int l = it->bitsRect.left;
					it->bitsRect.left = it->bitsRect.right;
					it->bitsRect.right = l;
				}
				if (it->bitsRect.top > it->bitsRect.bottom) {
					int t = it->bitsRect.top;
					it->bitsRect.top = it->bitsRect.bottom;
					it->bitsRect.bottom = t;
				}

				if (lsRect.left > lsRect.right) {
					int l = lsRect.left;
					lsRect.left = lsRect.right;
					lsRect.right = l;
				}
				if (lsRect.top > lsRect.bottom) {
					int t = lsRect.top;
					lsRect.top = lsRect.bottom;
					lsRect.bottom = t;
				}
				workerRect = lsRect;

				workerRect.clip(it->bitsRect);

				if (!workerRect.isEmpty()) {
					workerRect = lsRect;
					if (workerRect.left > workerRect.right) {
						int l = workerRect.left;
						workerRect.left = workerRect.right;
						workerRect.right = l;
					}
					if (workerRect.top > workerRect.bottom) {
						int t = workerRect.top;
						workerRect.top = workerRect.bottom;
						workerRect.bottom = t;
					}
					if (_screen->_upscaledHires != GFX_SCREEN_UPSCALED_640x400) {
						workerRect.extend(it->celRect);
					} else {
						workerRect.extend(it->bitsRect);
					}
				} else {
					_paint16->bitsShow(lsRect);
					workerRect = it->bitsRect;
				}
				if (_screen->_upscaledHires != GFX_SCREEN_UPSCALED_640x400) {
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsLeft), it->celRect.left);
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsTop), it->celRect.top);
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsRight), it->celRect.right);
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsBottom), it->celRect.bottom); /// UNDO TO HERE
				} else {
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsLeft), it->bitsRect.left);
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsTop), it->bitsRect.top);
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsRight), it->bitsRect.right);
					writeSelectorValue(_s->_segMan, it->object, SELECTOR(lsBottom), it->bitsRect.bottom);
				}
				// may get used for debugging
				//_paint16->frameRect(workerRect); // should be //!
				_paint16->bitsShow(workerRect);

				if (it->signal & kSignalHidden)
					it->signal |= kSignalRemoveView;
			}
		}
		// use this for debug purposes
		/*
	if (g_sci->backgroundIsVideo)
	{	
		reAnimate(_ports->_curPort->rect);
		//_screen->convertToRGB(_ports->_curPort->rect);
	}
	if (_screen->_upscaledHires == GFX_SCREEN_UPSCALED_640x400) {
		
		
	}*/
		//_screen->convertToRGB(_ports->_curPort->rect);
		
		reAnimate(_ports->_curPort->rect);
		// 2. Convert to RGB
		//_screen->convertToRGB(_ports->_curPort->rect);

		// 3. Copy to screen
		//g_system->copyRectToScreen(_screen->_rgbScreen + ((_ports->_curPort->rect.top * _screen->_displayWidth) + _ports->_curPort->rect.left) * _screen->_format.bytesPerPixel, _screen->_displayWidth * _screen->_format.bytesPerPixel, _ports->_curPort->rect.left, _ports->_curPort->rect.top, _ports->_curPort->rect.width(), _ports->_curPort->rect.height());
	}
	
}

void GfxAnimate::restoreAndDelete(int argc, reg_t *argv) {
	AnimateList::iterator it;
	const AnimateList::iterator end = _list.end();

	// This has to be done in a separate loop. At least in sq1 some .dispose
	// modifies FIXEDLOOP flag in signal for another object. In that case we
	// would overwrite the new signal with our version of the old signal.
	for (it = _list.begin(); it != end; ++it) {
		// Finally update signal
		writeSelectorValue(_s->_segMan, it->object, SELECTOR(signal), it->signal);
	}

	for (it = _list.reverse_begin(); it != end; --it) {
		// We read out signal here again, this is not by accident but to ensure
		// that we got an up-to-date signal
		it->signal = readSelectorValue(_s->_segMan, it->object, SELECTOR(signal));

		if ((it->signal & (kSignalNoUpdate | kSignalRemoveView)) == 0) {
			_paint16->bitsRestore(readSelector(_s->_segMan, it->object, SELECTOR(underBits)));
			writeSelectorValue(_s->_segMan, it->object, SELECTOR(underBits), 0);
		}

		if (it->signal & kSignalDisposeMe) {
			// Call .delete_ method of that object
			if (g_sci->stereoscopic) {
				if (g_sci->stereoRightEye) {
					invokeSelector(_s, it->object, SELECTOR(delete_), argc, argv, 0);
				}
			} else {
				invokeSelector(_s, it->object, SELECTOR(delete_), argc, argv, 0);
			}
		}
	}
}

void GfxAnimate::reAnimate(Common::Rect rect) {

	if (!_lastCastData.empty()) {
		AnimateArray::iterator it;
		AnimateArray::iterator end = _lastCastData.end();
		for (it = _lastCastData.begin(); it != end; ++it) {
			if (_screen->_upscaledHires == GFX_SCREEN_UPSCALED_640x400) {
				it->castHandle = _paint16->bitsSave(it->bitsRect, GFX_SCREEN_MASK_VISUAL | GFX_SCREEN_MASK_PRIORITY);
			} else {
				it->castHandle = _paint16->bitsSave(it->celRect, GFX_SCREEN_MASK_VISUAL | GFX_SCREEN_MASK_PRIORITY);
			}
			if (g_sci->enhanced_gfx_enabled) {
				Common::FSNode folder;

				if (ConfMan.hasKey("extrapath")) {
					char viewstrbuffer[32];
					int retVal, buf_size = 32;
					retVal = snprintf(viewstrbuffer, buf_size, "view.%u.%u.%u", it->viewId, it->loopNo, it->celNo);
					Common::String fn = viewstrbuffer;

					if (g_sci->stereoscopic && g_sci->stereoRightEye) {
						if (fileIsInExtraDIR((fn + ".reye.png").c_str()))
							fn += ".reye";
					}

					bool preloaded = false;
					if (it->viewpng == NULL) {
						if (viewsMap.size() > 0)
							for (viewsMapit = viewsMap.begin();
							     viewsMapit != viewsMap.end(); ++viewsMapit) {

								if (strcmp(viewsMapit->first.c_str(), (fn + ".png").c_str()) == 0) {

									//debug(viewsMapit->first.c_str());
									std::pair<Graphics::Surface *, const byte *> tmp = viewsMapit->second;
									it->viewpng = tmp.first;
									//debug("RELOADED FROM RAM");
									it->viewenh = tmp.second;
									if (it->viewenh) {
										preloaded = true;
										it->pixelsLength = it->viewpng->w * it->viewpng->h;
										it->viewEnhanced = true;
										it->enhancedIs256 = false;
									}
								}
							if (strcmp(viewsMapit->first.c_str(), (fn + "_256.png").c_str()) == 0) {

									//debug(viewsMapit->first.c_str());
									std::pair<Graphics::Surface *, const byte *> tmp = viewsMapit->second;
									it->viewpng = tmp.first;
									//debug("RELOADED FROM RAM");
									it->viewenh = tmp.second;
									if (it->viewenh) {
										preloaded = true;
										it->pixelsLength = it->viewpng->w * it->viewpng->h;
										it->viewEnhanced = true;
										it->enhancedIs256 = true;
									}
								}
							if (strcmp(viewsMapit->first.c_str(), (fn + "_256RP.png").c_str()) == 0) {

									//debug(viewsMapit->first.c_str());
									std::pair<Graphics::Surface *, const byte *> tmp = viewsMapit->second;
									it->viewpng = tmp.first;
									//debug("RELOADED FROM RAM");
									it->viewenh = tmp.second;
									if (it->viewenh) {
										preloaded = true;
										it->pixelsLength = it->viewpng->w * it->viewpng->h;
										it->viewEnhanced = true;
										it->enhancedIs256 = true;
									}
								}
							}
						if (!preloaded) {
							if (!extraDIRList.empty()) {
								if (fileIsInExtraDIR((fn + ".png").c_str())) {
									if (!it->viewEnhanced) {
										Common::String fileName = fn + ".png";
										Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
										if (!file) {
											//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
										} else {
											////debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
											Graphics::Surface *viewpngtmp = loadCelPNG(file);
											it->viewpng = viewpngtmp;
											if (it->viewpng) {
												const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
												it->viewenh = viewenhtmp;
												if (it->viewenh) {
													it->pixelsLength = it->viewpng->w * it->viewpng->h;
													it->viewEnhanced = true;
													it->enhancedIs256 = false;
													std::pair<Graphics::Surface *, const byte *> tmp;
													tmp.first = viewpngtmp;
													tmp.second = viewenhtmp;
													viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + ".png").c_str(), tmp));
													//debug(fn.c_str());
													//debug("LOADED FROM DISC");
												}
											}
										}
									}
								} else if (fileIsInExtraDIR((fn + "_256.png").c_str())) {
									if (!it->viewEnhanced) {
										Common::String fileName = fn + "_256.png";
										Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
										if (!file) {
											//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
										} else {
											////debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
											Graphics::Surface *viewpngtmp = loadCelPNGCLUT(file);
											it->viewpng = viewpngtmp;
											if (it->viewpng) {
												const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
												it->viewenh = viewenhtmp;
												if (it->viewenh) {
													it->pixelsLength = it->viewpng->w * it->viewpng->h;
													it->viewEnhanced = true;
													it->enhancedIs256 = true;
													std::pair<Graphics::Surface *, const byte *> tmp;
													tmp.first = viewpngtmp;
													tmp.second = viewenhtmp;
													viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + "_256.png").c_str(), tmp));
													//debug(fn.c_str());
													//debug("LOADED FROM DISC");
												}
											}
										}
									}
								} else if (fileIsInExtraDIR((fn + "_256RP.png").c_str())) {
									if (!it->viewEnhanced) {
										Common::String fileName = fn + "_256RP.png";
										Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
										if (!file) {
											//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
										} else {
											//debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
											Graphics::Surface *viewpngtmp = loadCelPNGCLUTOverride(file);
											it->viewpng = viewpngtmp;
											if (it->viewpng) {
												const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
												it->viewenh = viewenhtmp;
												if (it->viewenh) {
													it->pixelsLength = it->viewpng->w * it->viewpng->h;
													it->viewEnhanced = true;
													it->enhancedIs256 = true;
													std::pair<Graphics::Surface *, const byte *> tmp;
													tmp.first = viewpngtmp;
													tmp.second = viewenhtmp;
													viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + "_256RP.png").c_str(), tmp));
													//debug(fn.c_str());
													//debug("LOADED FROM DISC");
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			
			
			_paint16->drawCelNoUpdate(it->viewpng, it->viewenh, it->pixelsLength, it->viewEnhanced, it->enhancedIs256, it->viewId, it->loopNo, it->celNo, 0, it->celRect, it->priority, it->paletteNo, it->scaleX, it->scaleY);
			
		}
		_paint16->bitsShow(rect);
		// restoring
		while (it != _lastCastData.begin()) {		// FIXME: HACK, this iterator use is not very safe
			it--;
			_paint16->bitsRestore(it->castHandle);
		}
	} else {
		_paint16->bitsShow(rect);
	}
	
}


void GfxAnimate::addToPicDrawCels() {
	reg_t curObject;
	GfxView *view = nullptr;
	AnimateList::iterator it;
	const AnimateList::iterator end = _list.end();

	for (it = _list.begin(); it != end; ++it) {
		curObject = it->object;

		// Get the corresponding view
		view = _cache->getView(it->viewId);

		// kAddToPic does not do loop/cel-number fixups

		if (it->priority == -1)
			it->priority = _ports->kernelCoordinateToPriority(it->y);

		if (!view->isScaleable()) {
			// Laura Bow 2 specific - Check fill() below
			it->scaleSignal = 0;
			it->scaleY = it->scaleX = 128;
		}

		// Create rect according to coordinates and given cel
		if (it->scaleSignal & kScaleSignalDoScaling) {
			if (it->scaleSignal & kScaleSignalGlobalScaling) {
				applyGlobalScaling(it, view);
			}
			if (!it->viewEnhanced) {
				view->getCelScaledRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->scaleX, it->scaleY, it->celRect);
			} else {
				view->getCelScaledRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->scaleX, it->scaleY, it->celRect);
			}
			g_sci->_gfxCompare->setNSRect(curObject, it->celRect);
		} else {
			if (!it->viewEnhanced) {
				view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
				view->getCelRect(it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
			} else {
				view->getCelRectEnhanced(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->celRect);
				view->getCelRectEnhancedBits(it->viewpng, it->viewEnhanced, it->loopNo, it->celNo, it->x, it->y, it->z, it->bitsRect);
				it->bitsRect.clip(_ports->_curPort->rect);
			}
		}

		// draw corresponding cel
		g_sci->stereoRightEye = false;
		_paint16->drawCel(it->viewpng, it->viewenh, it->pixelsLength, it->viewEnhanced, it->enhancedIs256, view, it->loopNo, it->celNo, 0, it->celRect, it->priority, it->paletteNo, it->scaleX, it->scaleY);
		if (g_sci->stereoscopic) {
			g_sci->stereoRightEye = true;
			_paint16->drawCel(it->viewpng, it->viewenh, it->pixelsLength, it->viewEnhanced, it->enhancedIs256, view, it->loopNo, it->celNo, 0, it->celRect, it->priority, it->paletteNo, it->scaleX, it->scaleY);
			g_sci->stereoRightEye = false;
		}
		if (!(it->signal & kSignalIgnoreActor)) {
			it->celRect.top = CLIP<int16>(_ports->kernelPriorityToCoordinate(it->priority) - 1, it->celRect.top, it->celRect.bottom - 1);
			_paint16->fillRect(it->celRect, GFX_SCREEN_MASK_CONTROL, 0, 0, 15);
		}
	}
}

void GfxAnimate::addToPicDrawView(GuiResourceId viewId, int16 loopNo, int16 celNo, int16 x, int16 y, int16 priority, int16 control) {
	GfxView *view = _cache->getView(viewId);
	Common::Rect celRect;

	if (priority == -1)
		priority = _ports->kernelCoordinateToPriority(y);

	AnimateEntry listEntry;
	if (g_sci->enhanced_gfx_enabled) {
		Common::FSNode folder;
		if (ConfMan.hasKey("extrapath")) {
			char viewstrbuffer[32];
			int retVal, buf_size = 32;
			retVal = snprintf(viewstrbuffer, buf_size, "view.%u.%u.%u", viewId, loopNo, celNo);
			Common::String fn = viewstrbuffer;
			
			if (g_sci->stereoscopic && g_sci->stereoRightEye) {
				if (fileIsInExtraDIR((fn + ".reye.png").c_str()))
					fn += ".reye";
			}

			bool preloaded = false;
			if (g_sci->stereoscopic && g_sci->stereoRightEye) {
				if (fileIsInExtraDIR((fn + ".reye.png").c_str()))
					fn += ".reye";
			}
			if (listEntry.viewpng == NULL) {
				if (viewsMap.size() > 0)
					for (viewsMapit = viewsMap.begin();
					     viewsMapit != viewsMap.end(); ++viewsMapit) {

						if (strcmp(viewsMapit->first.c_str(), (fn + ".png").c_str()) == 0) {

							//debug(viewsMapit->first.c_str());
							std::pair<Graphics::Surface *, const byte *> tmp = viewsMapit->second;
							listEntry.viewpng = tmp.first;
							//debug("RELOADED FROM RAM");

							listEntry.viewenh = tmp.second;
							if (listEntry.viewenh) {
								preloaded = true;
								listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
								listEntry.viewEnhanced = true;
								listEntry.enhancedIs256 = (listEntry.viewpng->format != Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
							}
						}
					}
				if (!preloaded) {
					if (!extraDIRList.empty()) {
						if (fileIsInExtraDIR((fn + ".png").c_str())) {
							if (!listEntry.viewEnhanced) {
								Common::String fileName = fn + ".png";
								Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
								if (!file) {
									//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
								} else {
									////debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
									Graphics::Surface *viewpngtmp = loadCelPNG(file);
									listEntry.viewpng = viewpngtmp;
									if (listEntry.viewpng) {
										const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
										listEntry.viewenh = viewenhtmp;
										if (listEntry.viewenh) {
											listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
											listEntry.viewEnhanced = true;
											listEntry.enhancedIs256 = (listEntry.viewpng->format != Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
											std::pair<Graphics::Surface *, const byte *> tmp;
											tmp.first = viewpngtmp;
											tmp.second = viewenhtmp;
											viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + ".png").c_str(), tmp));
											//debug(fn.c_str());
											//debug("LOADED FROM DISC");
										}
									}
								}
							}
						} else if (fileIsInExtraDIR((fn + "_256.png").c_str())) {
							if (!listEntry.viewEnhanced) {
								Common::String fileName = fn + "_256.png";
								Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
								if (!file) {
									//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
								} else {
									////debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
									Graphics::Surface *viewpngtmp = loadCelPNGCLUT(file);
									listEntry.viewpng = viewpngtmp;
									if (listEntry.viewpng) {
										const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
										listEntry.viewenh = viewenhtmp;
										if (listEntry.viewenh) {
											listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
											listEntry.viewEnhanced = true;
											listEntry.enhancedIs256 = true;
											std::pair<Graphics::Surface *, const byte *> tmp;
											tmp.first = viewpngtmp;
											tmp.second = viewenhtmp;
											viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + "_256.png").c_str(), tmp));
											//debug(fn.c_str());
											//debug("LOADED FROM DISC");
										}
									}
								}
							}
						} else if (fileIsInExtraDIR((fn + "_256RP.png").c_str())) {
							if (!listEntry.viewEnhanced) {
								Common::String fileName = fn + "_256RP.png";
								Common::SeekableReadStream *file = SearchMan.createReadStreamForMember(fileName);
								if (!file) {
									//debug("Enhanced Bitmap %s DOES NOT EXIST, yet would have been loaded.. 2", fileName.c_str());
								} else {
									//debug("Enhanced Bitmap %s EXISTS, and has been loaded..", fileName.c_str());
									Graphics::Surface *viewpngtmp = loadCelPNGCLUTOverride(file);
									listEntry.viewpng = viewpngtmp;
									if (listEntry.viewpng) {
										const byte *viewenhtmp = (const byte *)viewpngtmp->getPixels();
										listEntry.viewenh = viewenhtmp;
										if (listEntry.viewenh) {
											listEntry.pixelsLength = listEntry.viewpng->w * listEntry.viewpng->h;
											listEntry.viewEnhanced = true;
											listEntry.enhancedIs256 = true;
											std::pair<Graphics::Surface *, const byte *> tmp;
											tmp.first = viewpngtmp;
											tmp.second = viewenhtmp;
											viewsMap.insert(std::pair<std::string, std::pair<Graphics::Surface *, const byte *> >((fn + "_256RP.png").c_str(), tmp));
											//debug(fn.c_str());
											//debug("LOADED FROM DISC");
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	    // Create rect according to coordinates and given cel
	    view->getCelRectEnhanced(listEntry.viewpng, listEntry.viewEnhanced,loopNo, celNo, x, y, 0, celRect);
	_paint16->drawCel(listEntry.viewpng, listEntry.viewenh, listEntry.pixelsLength, listEntry.viewEnhanced, listEntry.enhancedIs256, view, loopNo, celNo, 0, celRect, priority, 0);

	if (control != -1) {
		celRect.top = CLIP<int16>(_ports->kernelPriorityToCoordinate(priority) - 1, celRect.top, celRect.bottom - 1);
		_paint16->fillRect(celRect, GFX_SCREEN_MASK_CONTROL, 0, 0, control);
	}
}


void GfxAnimate::animateShowPic() {
	Port *picPort = _ports->_picWind;
	Common::Rect picRect = picPort->rect;
	bool previousCursorState = _cursor->isVisible();

	if (previousCursorState)
		_cursor->kernelHide();
	// Adjust picRect to become relative to screen
	picRect.translate(picPort->left, picPort->top);
	_transitions->doit(picRect);
	if (previousCursorState)
		_cursor->kernelShow();
}

void GfxAnimate::kernelAnimate(reg_t listReference, bool cycle, int argc, reg_t *argv) {
	// If necessary, delay this kAnimate for a running PalVary.
	// See delayForPalVaryWorkaround() for details.
	if (_screen->_picNotValid)
		_palette->delayForPalVaryWorkaround();

	byte old_picNotValid = _screen->_picNotValid;

	if (getSciVersion() >= SCI_VERSION_1_1)
		_palette->palVaryUpdate();

	if (listReference.isNull()) {
		disposeLastCast();
		if (_screen->_picNotValid)
			animateShowPic();
		return;
	}

	List *list = _s->_segMan->lookupList(listReference);
	if (!list)
		error("kAnimate called with non-list as parameter");

	if (cycle) {
		if (!invoke(list, argc, argv))
			return;

		// Look up the list again, as it may have been modified
		list = _s->_segMan->lookupList(listReference);
	}

	Port *oldPort = _ports->setPort((Port *)_ports->_picWind);
	if (g_sci->stereoscopic) {
		if (!g_sci->stereoRightEye) {
			makeSortedList(list);			
		} else {
			disposeLastCast();
		}
	} else {
		disposeLastCast();
		makeSortedList(list);
	}
	
	fill(old_picNotValid);

	if (old_picNotValid) {
		// beginUpdate()/endUpdate() were introduced SCI1.
		// Calling those for SCI0 will work most of the time but breaks minor
		// stuff like percentage bar of qfg1ega at the character skill screen.
		if (getSciVersion() >= SCI_VERSION_1_EGA_ONLY)
			_ports->beginUpdate(_ports->_picWind);
		update();
		if (getSciVersion() >= SCI_VERSION_1_EGA_ONLY)
			_ports->endUpdate(_ports->_picWind);
	}

	drawCels();

	if (_screen->_picNotValid)
		animateShowPic();

	updateScreen(old_picNotValid);
	restoreAndDelete(argc, argv);

	// We update the screen here as well, some scenes like EQ1 credits run w/o calling kGetEvent thus we wouldn't update
	//  screen at all
	g_sci->getEventManager()->updateScreen();

	_ports->setPort(oldPort);

	// Now trigger speed throttler
	_s->_throttleTrigger = true;
}

void GfxAnimate::addToPicSetPicNotValid() {
	if (getSciVersion() <= SCI_VERSION_1_EARLY)
		_screen->_picNotValid = 1;
	else
		_screen->_picNotValid = 2;
}

void GfxAnimate::kernelAddToPicList(reg_t listReference, int argc, reg_t *argv) {
	List *list;

	_ports->setPort((Port *)_ports->_picWind);

	list = _s->_segMan->lookupList(listReference);
	if (!list)
		error("kAddToPic called with non-list as parameter");

	makeSortedList(list);
	addToPicDrawCels();

	addToPicSetPicNotValid();
}

void GfxAnimate::kernelAddToPicView(GuiResourceId viewId, int16 loopNo, int16 celNo, int16 x, int16 y, int16 priority, int16 control) {
	_ports->setPort((Port *)_ports->_picWind);
	addToPicDrawView(viewId, loopNo, celNo, x, y, priority, control);
	addToPicSetPicNotValid();
}

void GfxAnimate::printAnimateList(Console *con) {
	AnimateList::iterator it;
	const AnimateList::iterator end = _list.end();

	for (it = _list.begin(); it != end; ++it) {
		Script *scr = _s->_segMan->getScriptIfLoaded(it->object.getSegment());
		int16 scriptNo = scr ? scr->getScriptNumber() : -1;

		con->debugPrintf("%04x:%04x (%s), script %d, view %d (%d, %d), pal %d, "
			"at %d, %d, scale %d, %d / %d (z: %d, prio: %d, shown: %d, signal: %d)\n",
			PRINT_REG(it->object), _s->_segMan->getObjectName(it->object),
			scriptNo, it->viewId, it->loopNo, it->celNo, it->paletteNo,
			it->x, it->y, it->scaleX, it->scaleY, it->scaleSignal,
			it->z, it->priority, it->showBitsFlag, it->signal);
	}
}

} // End of namespace Sci
