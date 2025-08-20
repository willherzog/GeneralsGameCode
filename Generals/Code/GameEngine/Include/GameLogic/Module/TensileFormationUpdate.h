/*
**	Command & Conquer Generals(tm)
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

// FILE: TensileFormationUpdate.h ////////////////////////////////////////////////////////////////////////////
// Author: Mark Lorenzen, November 2002
// Desc:   Springy formation movement like that of say, an avalanche
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __TENSILEFORMATIONUPDATE_H_
#define __TENSILEFORMATIONUPDATE_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
struct FieldParse;

//-------------------------------------------------------------------------------------------------
class TensileFormationUpdateModuleData: public UpdateModuleData
{

public:

	TensileFormationUpdateModuleData( void );

	static void buildFieldParse(MultiIniFieldParse& p);

	Bool m_enabled;							///< enabled
	AudioEventRTS				m_crackSound;

};

//-------------------------------------------------------------------------------------------------
class TensileFormationUpdate : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( TensileFormationUpdate, "TensileFormationUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( TensileFormationUpdate, TensileFormationUpdateModuleData )

public:

	TensileFormationUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	void setEnabled( Bool enabled ) { m_enabled = enabled; }  ///< enable/disable formation
	virtual UpdateSleepTime update();	///< Deciding whether or not to make new guys

protected:

	void propagateDislodgement( Bool enabled );
	void initLinks( void );

	struct TensileLink
	{
		ObjectID id;
		Coord3D tensor;
	};

	TensileLink m_links[4];//standard textile algorithm
	Coord3D m_inertia;
	Bool m_enabled;			///< enabled
	Bool m_linksInited;
	UnsignedInt m_motionlessCounter;
	UnsignedInt m_life;
	Real m_lowestSlideElevation;

	AudioEventRTS				m_crackSound;

};

#endif  // end __TENSILEFORMATIONUPDATE_H_
