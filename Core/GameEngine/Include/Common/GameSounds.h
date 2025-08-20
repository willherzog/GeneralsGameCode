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

//----------------------------------------------------------------------------
//
//                       Westwood Studios Pacific.
//
//                       Confidential Information
//                Copyright (C) 2001 - All Rights Reserved
//
//----------------------------------------------------------------------------
//
// Project:    RTS 3
//
// File name:  Common/GameSounds.h
//
// Created:    5/02/01
//
//----------------------------------------------------------------------------

#pragma once
#ifndef __COMMON_GAMESOUNDS_H_
#define __COMMON_GAMESOUNDS_H_

#include "Common/SubsystemInterface.h"
#include "Common/GameAudio.h"
#include "Common/GameType.h"

// Forward declarations
class AudioEventRTS;

class SoundManager : public SubsystemInterface
{
	public:
		SoundManager();
		virtual ~SoundManager();

		virtual void init( void );										///< Initializes the sounds system
		virtual void postProcessLoad();
		virtual void update( void );									///< Services sounds tasks. Called by AudioInterface
		virtual void reset( void );										///< Reset the sounds system

		virtual void loseFocus( void );								///< Called when application loses focus
		virtual void regainFocus( void );							///< Called when application regains focus

		virtual void setListenerPosition( const Coord3D *position );	///< Set the listener position for map3DSound() calculations
		virtual void setViewRadius( Real viewRadius );///< Sets the radius of the view from the center of the screen in world coordinate units
		virtual void setCameraAudibleDistance( Real audibleDistance );
		virtual Real getCameraAudibleDistance( void );

		virtual void addAudioEvent(AudioEventRTS *&eventToAdd);	// pre-copied

		virtual void notifyOf2DSampleStart( void );
		virtual void notifyOf3DSampleStart( void );

		virtual void notifyOf2DSampleCompletion( void );
		virtual void notifyOf3DSampleCompletion( void );

		virtual Int getAvailableSamples( void );
		virtual Int getAvailable3DSamples( void );

		// empty string means that this sound wasn't found or some error occurred. CHECK FOR EMPTY STRING.
		virtual AsciiString getFilenameForPlayFromAudioEvent( const AudioEventRTS *eventToGetFrom );

		// called by this class and MilesAudioManager to determine if a sound can still be played
		virtual Bool canPlayNow( AudioEventRTS *event );

	protected:
		virtual Bool violatesVoice( AudioEventRTS *event );
		virtual Bool isInterrupting( AudioEventRTS *event );


	protected:
		UnsignedInt m_num2DSamples;
		UnsignedInt m_num3DSamples;

		UnsignedInt m_numPlaying2DSamples;
		UnsignedInt m_numPlaying3DSamples;
};

#endif // __COMMON_GAMESOUNDS_H_

