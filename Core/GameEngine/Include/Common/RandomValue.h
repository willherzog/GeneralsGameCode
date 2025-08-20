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

// RandomValue.h
// Random number generation system
// Author: Michael S. Booth, January 1998

#pragma once

#ifndef _RANDOM_VALUE_H_
#define _RANDOM_VALUE_H_

#include "Lib/BaseType.h"

extern void InitRandom( void );
extern void InitRandom( UnsignedInt seed );
extern void InitGameLogicRandom( UnsignedInt seed ); ///< Set the GameLogic seed to a known value at game start
extern UnsignedInt GetGameLogicRandomSeed( void );   ///< Get the seed (used for replays)
extern UnsignedInt GetGameLogicRandomSeedCRC( void );///< Get the seed (used for CRCs)

//--------------------------------------------------------------------------------------------------------------

#endif // _RANDOM_VALUE_H_
