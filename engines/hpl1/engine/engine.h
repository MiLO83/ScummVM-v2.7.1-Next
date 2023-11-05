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

#ifndef HPL1_ENGINE_H
#define HPL1_ENGINE_H

#include "hpl1/engine/ai/AI.h"
#include "hpl1/engine/ai/AINodeContainer.h"
#include "hpl1/engine/ai/AINodeGenerator.h"
#include "hpl1/engine/ai/AStar.h"
#include "hpl1/engine/ai/StateMachine.h"
#include "hpl1/engine/game/Game.h"
#include "hpl1/engine/game/GameTypes.h"
#include "hpl1/engine/game/low_level_game_setup.h"
#include "hpl1/engine/game/SaveGame.h"
#include "hpl1/engine/game/ScriptFuncs.h"
#include "hpl1/engine/game/Updateable.h"
#include "hpl1/engine/game/Updater.h"
#include "hpl1/engine/graphics/Animation.h"
#include "hpl1/engine/graphics/AnimationTrack.h"
#include "hpl1/engine/graphics/BackgroundImage.h"
#include "hpl1/engine/graphics/Beam.h"
#include "hpl1/engine/graphics/BillBoard.h"
#include "hpl1/engine/graphics/bitmap2D.h"
#include "hpl1/engine/graphics/Bone.h"
#include "hpl1/engine/graphics/BoneState.h"
#include "hpl1/engine/graphics/Color.h"
#include "hpl1/engine/graphics/font_data.h"
#include "hpl1/engine/graphics/GPUProgram.h"
#include "hpl1/engine/graphics/GfxObject.h"
#include "hpl1/engine/graphics/Graphics.h"
#include "hpl1/engine/graphics/GraphicsDrawer.h"
#include "hpl1/engine/graphics/GraphicsTypes.h"
#include "hpl1/engine/graphics/ImageEntityData.h"
#include "hpl1/engine/graphics/LowLevelGraphics.h"
#include "hpl1/engine/graphics/LowLevelPicture.h"
#include "hpl1/engine/graphics/Material.h"
#include "hpl1/engine/graphics/MaterialHandler.h"
#include "hpl1/engine/graphics/Material_Additive.h"
#include "hpl1/engine/graphics/Material_Alpha.h"
#include "hpl1/engine/graphics/Material_BaseLight.h"
#include "hpl1/engine/graphics/Material_Bump.h"
#include "hpl1/engine/graphics/Material_BumpColorSpec.h"
#include "hpl1/engine/graphics/Material_BumpSpec.h"
#include "hpl1/engine/graphics/Material_BumpSpec2D.h"
#include "hpl1/engine/graphics/Material_Diffuse.h"
#include "hpl1/engine/graphics/Material_Diffuse2D.h"
#include "hpl1/engine/graphics/Material_DiffuseAdditive2D.h"
#include "hpl1/engine/graphics/Material_DiffuseAlpha2D.h"
#include "hpl1/engine/graphics/Material_DiffuseSpec.h"
#include "hpl1/engine/graphics/Material_EnvMap_Reflect.h"
#include "hpl1/engine/graphics/Material_Fallback01_BaseLight.h"
#include "hpl1/engine/graphics/Material_Fallback02_BaseLight.h"
#include "hpl1/engine/graphics/Material_Flat.h"
#include "hpl1/engine/graphics/Material_FontNormal.h"
#include "hpl1/engine/graphics/Material_Modulative.h"
#include "hpl1/engine/graphics/Material_ModulativeX2.h"
#include "hpl1/engine/graphics/Material_Smoke2D.h"
#include "hpl1/engine/graphics/Material_Water.h"
#include "hpl1/engine/graphics/Mesh.h"
#include "hpl1/engine/graphics/Mesh2d.h"
#include "hpl1/engine/graphics/MeshCreator.h"
#include "hpl1/engine/graphics/OcclusionQuery.h"
#include "hpl1/engine/graphics/ParticleEmitter.h"
#include "hpl1/engine/graphics/ParticleEmitter2D.h"
#include "hpl1/engine/graphics/ParticleEmitter3D.h"
#include "hpl1/engine/graphics/ParticleEmitter3D_UserData.h"
#include "hpl1/engine/graphics/ParticleSystem3D.h"
#include "hpl1/engine/graphics/RenderList.h"
#include "hpl1/engine/graphics/RenderObject2D.h"
#include "hpl1/engine/graphics/RenderState.h"
#include "hpl1/engine/graphics/Renderable.h"
#include "hpl1/engine/graphics/Renderer2D.h"
#include "hpl1/engine/graphics/Renderer3D.h"
#include "hpl1/engine/graphics/RendererPostEffects.h"
#include "hpl1/engine/graphics/Skeleton.h"
#include "hpl1/engine/graphics/SubMesh.h"
#include "hpl1/engine/graphics/Texture.h"
#include "hpl1/engine/graphics/VertexBuffer.h"
#include "hpl1/engine/graphics/VideoStream.h"
#include "hpl1/engine/gui/Gui.h"
#include "hpl1/engine/gui/GuiGfxElement.h"
#include "hpl1/engine/gui/GuiMaterial.h"
#include "hpl1/engine/gui/GuiMaterialBasicTypes.h"
#include "hpl1/engine/gui/GuiPopUp.h"
#include "hpl1/engine/gui/GuiPopUpMessageBox.h"
#include "hpl1/engine/gui/GuiSet.h"
#include "hpl1/engine/gui/GuiSkin.h"
#include "hpl1/engine/gui/GuiTypes.h"
#include "hpl1/engine/gui/Widget.h"
#include "hpl1/engine/gui/WidgetBaseClasses.h"
#include "hpl1/engine/gui/WidgetButton.h"
#include "hpl1/engine/gui/WidgetCheckBox.h"
#include "hpl1/engine/gui/WidgetComboBox.h"
#include "hpl1/engine/gui/WidgetFrame.h"
#include "hpl1/engine/gui/WidgetImage.h"
#include "hpl1/engine/gui/WidgetLabel.h"
#include "hpl1/engine/gui/WidgetListBox.h"
#include "hpl1/engine/gui/WidgetSlider.h"
#include "hpl1/engine/gui/WidgetTextBox.h"
#include "hpl1/engine/gui/WidgetWindow.h"
#include "hpl1/engine/haptic/Haptic.h"
#include "hpl1/engine/haptic/HapticForce.h"
#include "hpl1/engine/haptic/HapticShape.h"
#include "hpl1/engine/haptic/HapticSurface.h"
#include "hpl1/engine/haptic/HapticTypes.h"
#include "hpl1/engine/haptic/LowLevelHaptic.h"
#include "hpl1/engine/impl/tinyXML/tinyxml.h"
#include "hpl1/engine/input/Action.h"
#include "hpl1/engine/input/ActionHaptic.h"
#include "hpl1/engine/input/ActionKeyboard.h"
#include "hpl1/engine/input/ActionMouseButton.h"
#include "hpl1/engine/input/Input.h"
#include "hpl1/engine/input/InputDevice.h"
#include "hpl1/engine/input/InputTypes.h"
#include "hpl1/engine/input/Keyboard.h"
#include "hpl1/engine/input/LowLevelInput.h"
#include "hpl1/engine/input/Mouse.h"
#include "hpl1/engine/math/BoundingVolume.h"
#include "hpl1/engine/math/Frustum.h"
#include "hpl1/engine/math/Math.h"
#include "hpl1/engine/math/MathTypes.h"
#include "hpl1/engine/math/hplMatrix.h"
#include "hpl1/engine/math/MeshTypes.h"
#include "hpl1/engine/math/PidController.h"
#include "hpl1/engine/math/Quaternion.h"
#include "hpl1/engine/math/Spring.h"
#include "hpl1/engine/math/Vector2.h"
#include "hpl1/engine/math/Vector3.h"
#include "hpl1/engine/physics/Body2D.h"
#include "hpl1/engine/physics/CharacterBody.h"
#include "hpl1/engine/physics/CollideData.h"
#include "hpl1/engine/physics/CollideData2D.h"
#include "hpl1/engine/physics/CollideShape.h"
#include "hpl1/engine/physics/Collider2D.h"
#include "hpl1/engine/physics/LowLevelPhysics.h"
#include "hpl1/engine/physics/Physics.h"
#include "hpl1/engine/physics/PhysicsBody.h"
#include "hpl1/engine/physics/PhysicsController.h"
#include "hpl1/engine/physics/PhysicsJoint.h"
#include "hpl1/engine/physics/PhysicsJointBall.h"
#include "hpl1/engine/physics/PhysicsJointHinge.h"
#include "hpl1/engine/physics/PhysicsJointScrew.h"
#include "hpl1/engine/physics/PhysicsJointSlider.h"
#include "hpl1/engine/physics/PhysicsMaterial.h"
#include "hpl1/engine/physics/PhysicsWorld.h"
#include "hpl1/engine/physics/SurfaceData.h"
#include "hpl1/engine/resources/AnimationManager.h"
#include "hpl1/engine/resources/ConfigFile.h"
#include "hpl1/engine/resources/EntityLoader_Object.h"
#include "hpl1/engine/resources/FileSearcher.h"
#include "hpl1/engine/resources/FontManager.h"
#include "hpl1/engine/resources/FrameBase.h"
#include "hpl1/engine/resources/FrameBitmap.h"
#include "hpl1/engine/resources/FrameTexture.h"
#include "hpl1/engine/resources/GpuProgramManager.h"
#include "hpl1/engine/resources/ImageEntityManager.h"
#include "hpl1/engine/resources/ImageManager.h"
#include "hpl1/engine/resources/LanguageFile.h"
#include "hpl1/engine/resources/low_level_resources.h"
#include "hpl1/engine/resources/MaterialManager.h"
#include "hpl1/engine/resources/MeshLoader.h"
#include "hpl1/engine/resources/MeshLoaderHandler.h"
#include "hpl1/engine/resources/MeshManager.h"
#include "hpl1/engine/resources/ParticleManager.h"
#include "hpl1/engine/resources/ResourceBase.h"
#include "hpl1/engine/resources/ResourceImage.h"
#include "hpl1/engine/resources/ResourceManager.h"
#include "hpl1/engine/resources/Resources.h"
#include "hpl1/engine/resources/ResourcesTypes.h"
#include "hpl1/engine/resources/ScriptManager.h"
#include "hpl1/engine/resources/SoundEntityManager.h"
#include "hpl1/engine/resources/SoundManager.h"
#include "hpl1/engine/resources/TextureManager.h"
#include "hpl1/engine/resources/TileSetManager.h"
#include "hpl1/engine/resources/VideoManager.h"
#include "hpl1/engine/scene/AnimationState.h"
#include "hpl1/engine/scene/Area2D.h"
#include "hpl1/engine/scene/Camera.h"
#include "hpl1/engine/scene/Camera2D.h"
#include "hpl1/engine/scene/Camera3D.h"
#include "hpl1/engine/scene/ColliderEntity.h"
#include "hpl1/engine/scene/Entity.h"
#include "hpl1/engine/scene/Entity2D.h"
#include "hpl1/engine/scene/Entity3D.h"
#include "hpl1/engine/scene/GridMap2D.h"
#include "hpl1/engine/scene/ImageEntity.h"
#include "hpl1/engine/scene/Light.h"
#include "hpl1/engine/scene/Light2D.h"
#include "hpl1/engine/scene/Light2DPoint.h"
#include "hpl1/engine/scene/Light3D.h"
#include "hpl1/engine/scene/Light3DPoint.h"
#include "hpl1/engine/scene/Light3DSpot.h"
#include "hpl1/engine/scene/MeshEntity.h"
#include "hpl1/engine/scene/MultiImageEntity.h"
#include "hpl1/engine/scene/Node.h"
#include "hpl1/engine/scene/Node2D.h"
#include "hpl1/engine/scene/Node3D.h"
#include "hpl1/engine/scene/NodeState.h"
#include "hpl1/engine/scene/PortalContainer.h"
#include "hpl1/engine/scene/RenderableContainer.h"
#include "hpl1/engine/scene/Scene.h"
#include "hpl1/engine/scene/SectorVisibility.h"
#include "hpl1/engine/scene/SoundEntity.h"
#include "hpl1/engine/scene/SoundSource.h"
#include "hpl1/engine/scene/SubMeshEntity.h"
#include "hpl1/engine/scene/Tile.h"
#include "hpl1/engine/scene/TileData.h"
#include "hpl1/engine/scene/TileLayer.h"
#include "hpl1/engine/scene/TileMap.h"
#include "hpl1/engine/scene/TileMapIt.h"
#include "hpl1/engine/scene/TileMapLineIt.h"
#include "hpl1/engine/scene/TileMapRectIt.h"
#include "hpl1/engine/scene/TileSet.h"
#include "hpl1/engine/scene/World2D.h"
#include "hpl1/engine/scene/World3D.h"
#include "hpl1/engine/sound/LowLevelSound.h"
#include "hpl1/engine/sound/MusicHandler.h"
#include "hpl1/engine/sound/Sound.h"
#include "hpl1/engine/sound/SoundChannel.h"
#include "hpl1/engine/sound/SoundData.h"
#include "hpl1/engine/sound/SoundEntityData.h"
#include "hpl1/engine/sound/SoundEnvironment.h"
#include "hpl1/engine/sound/SoundHandler.h"
#include "hpl1/engine/system/BinTree.h"
#include "hpl1/engine/system/Container.h"
#include "hpl1/engine/system/LogicTimer.h"
#include "hpl1/engine/system/low_level_system.h"
#include "hpl1/engine/system/MemoryManager.h"
#include "hpl1/engine/system/Script.h"
#include "hpl1/engine/system/SerializeClass.h"
#include "hpl1/engine/system/String.h"
#include "hpl1/engine/system/System.h"
#include "hpl1/engine/system/SystemTypes.h"

#endif