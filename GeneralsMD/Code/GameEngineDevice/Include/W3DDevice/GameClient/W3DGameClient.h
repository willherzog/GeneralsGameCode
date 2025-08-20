/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: W3DGameClient.h ///////////////////////////////////////////////////
//
// W3D implementation of the game interface.  The GameClient is
// responsible for maintaining our drawbles, handling our GUI, and creating
// the display ... basically the Client if this were a Client/Server game.
//
// Author: Colin Day, April 2001
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __W3DGAMEINTERFACE_H_
#define __W3DGAMEINTERFACE_H_

// SYSTEM INCLUDES ////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////
#include "GameClient/GameClient.h"
#include "W3DDevice/GameClient/W3DParticleSys.h"
#include "W3DDevice/GameClient/W3DDisplay.h"
#include "W3DDevice/GameClient/W3DInGameUI.h"
#include "W3DDevice/GameClient/W3DTerrainVisual.h"
#include "W3DDevice/GameClient/W3DGameWindowManager.h"
#include "W3DDevice/GameClient/W3DGameFont.h"
#include "W3DDevice/GameClient/W3DDisplayStringManager.h"
#include "VideoDevice/Bink/BinkVideoPlayer.h"
#ifdef RTS_HAS_FFMPEG
#include "VideoDevice/FFmpeg/FFmpegVideoPlayer.h"
#endif
#include "Win32Device/GameClient/Win32DIKeyboard.h"
#include "Win32Device/GameClient/Win32DIMouse.h"
#include "Win32Device/GameClient/Win32Mouse.h"
#include "W3DDevice/GameClient/W3DMouse.h"
#include "W3DDevice/GameClient/W3DSnow.h"

class ThingTemplate;

extern Win32Mouse *TheWin32Mouse;

///////////////////////////////////////////////////////////////////////////////
// PROTOTYPES /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// W3DGameClient -----------------------------------------------------------
/** The W3DGameClient singleton */
//-----------------------------------------------------------------------------
class W3DGameClient : public GameClient
{

public:

	W3DGameClient();
	virtual ~W3DGameClient();

	/// given a type, create a drawable
	virtual Drawable *friend_createDrawable( const ThingTemplate *thing, DrawableStatus statusBits = DRAWABLE_STATUS_NONE );

	virtual void init( void );		///< initialize resources
	virtual void update( void );  ///< per frame update
	virtual void reset( void );   ///< reset system

	virtual void addScorch(const Coord3D *pos, Real radius, Scorches type);
	virtual void createRayEffectByTemplate( const Coord3D *start, const Coord3D *end, const ThingTemplate* tmpl );  ///< create effect needing start and end location
	//virtual Bool getBonePos(Drawable *draw, AsciiString boneName, Coord3D* pos, Matrix3D* transform) const;

	virtual void setTimeOfDay( TimeOfDay tod );							///< Tell all the drawables what time of day it is now

	//---------------------------------------------------------------------------
	virtual void setTeamColor( Int red, Int green, Int blue );  ///< @todo superhack for demo, remove!!!
	virtual void adjustLOD( Int adj ); ///< @todo hack for evaluation, remove.
	virtual void notifyTerrainObjectMoved(Object *obj);

protected:

	virtual Keyboard *createKeyboard( void );								///< factory for the keyboard
	virtual Mouse *createMouse( void );											///< factory for the mouse

	/// factory for creating TheDisplay
	virtual Display *createGameDisplay( void ) { return NEW W3DDisplay; }

	/// factory for creating TheInGameUI
	virtual InGameUI *createInGameUI( void ) { return NEW W3DInGameUI; }

	/// factory for creating the window manager
	virtual GameWindowManager *createWindowManager( void ) { return NEW W3DGameWindowManager; }

	/// factory for creating the font library
	virtual FontLibrary *createFontLibrary( void ) { return NEW W3DFontLibrary; }

  /// Manager for display strings
	virtual DisplayStringManager *createDisplayStringManager( void ) { return NEW W3DDisplayStringManager; }
#ifdef RTS_HAS_FFMPEG
	virtual VideoPlayerInterface *createVideoPlayer( void ) { return NEW FFmpegVideoPlayer; }
#else
	virtual VideoPlayerInterface *createVideoPlayer( void ) { return NEW BinkVideoPlayer; }
#endif
	/// factory for creating the TerrainVisual
	virtual TerrainVisual *createTerrainVisual( void ) { return NEW W3DTerrainVisual; }

	/// factory for creating the snow manager
	virtual SnowManager *createSnowManager( void ) { return NEW W3DSnowManager; }

	virtual void setFrameRate(Real msecsPerFrame) { TheW3DFrameLengthInMsec = msecsPerFrame; }

};  // end class W3DGameClient

inline Keyboard *W3DGameClient::createKeyboard( void ) { return NEW DirectInputKeyboard; }
inline Mouse *W3DGameClient::createMouse( void )
{
	//return new DirectInputMouse;
	Win32Mouse * mouse = NEW W3DMouse;
	TheWin32Mouse = mouse;   ///< global cheat for the WndProc()
	return mouse;
}

#endif  // end __W3DGAMEINTERFACE_H_
