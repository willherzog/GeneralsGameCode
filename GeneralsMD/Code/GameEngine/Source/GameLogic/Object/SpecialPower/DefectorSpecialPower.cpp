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

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// FILE: DefectorSpecialPower.cpp
// Author: Mark Lorenzen, JULY 2002
// Desc:   General can click command cursor on any enemy, and it becomes his
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Player.h"
#include "Common/SpecialPower.h"
#include "Common/Team.h"
#include "Common/Xfer.h"

#include "GameLogic/Module/DefectorSpecialPower.h"
#include "GameLogic/Object.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
DefectorSpecialPowerModuleData::DefectorSpecialPowerModuleData( void )
{

	m_fatCursorRadius = 0.0f;

}  // end DefectorSpecialPowerModuleData

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

	//static

 void DefectorSpecialPowerModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	SpecialPowerModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "FatCursorRadius", INI::parseReal, NULL, offsetof( DefectorSpecialPowerModuleData, m_fatCursorRadius ) },
		{ 0, 0, 0, 0 }
	};
	p.add(dataFieldParse);

}  // end buildFieldParse

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
DefectorSpecialPower::DefectorSpecialPower( Thing *thing, const ModuleData *moduleData )
												: SpecialPowerModule( thing, moduleData )
{

}  // end DefectorSpecialPower

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
DefectorSpecialPower::~DefectorSpecialPower( void )
{

}  // end ~DefectorSpecialPower


// ------------------------------------------------------------------------------------------------

void DefectorSpecialPower::doSpecialPowerAtLocation( const Coord3D *loc, Real angle, UnsignedInt commandOptions )
{
	if (getObject()->isDisabled())
		return;

	// only allowed at objects
	return;
}

// ------------------------------------------------------------------------------------------------
void DefectorSpecialPower::doSpecialPowerAtObject( Object *objectToMakeDefector, UnsignedInt commandOptions )
{
	if (getObject()->isDisabled())
		return;

	// sanity check
	if (!objectToMakeDefector)
	{
		return;
	}

	// another sanity check
	const Object *self = getObject();
	if (!self)
	{
		return;
	}

	// call the base class action cause we are *EXTENDING* functionality
  SpecialPowerModule::doSpecialPowerAtObject( objectToMakeDefector, commandOptions );

	//AIUpdateInterface *hisAI = objectToMakeDefector->getAIUpdateInterface();
	//if (hisAI)
	//{
		// how do I get at SpecialPowerTemplate::getDetectionTime() from here?
		const SpecialPowerTemplate *specPowTemp = getSpecialPowerTemplate();
		UnsignedInt time = specPowTemp->getDetectionTime();


		objectToMakeDefector->defect(self->getControllingPlayer()->getDefaultTeam(), time );// @todo lorenzen hook into the new AIUpdateI methods
	//}

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DefectorSpecialPower::crc( Xfer *xfer )
{

	// extend base class
	SpecialPowerModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void DefectorSpecialPower::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	SpecialPowerModule::xfer( xfer );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DefectorSpecialPower::loadPostProcess( void )
{

	// extend base class
	SpecialPowerModule::loadPostProcess();

}  // end loadPostProcess
