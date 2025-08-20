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
// FILE: SabotageMilitaryFactoryCrateCollide.h
// Author: Kris Morness, June 2003
// Desc:   A crate (actually a saboteur - mobile crate) that temporarily disables the target factory
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef SABOTAGE_MILITARY_FACTORY_CRATE_COLLIDE_H_
#define SABOTAGE_MILITARY_FACTORY_CRATE_COLLIDE_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/Module.h"
#include "GameLogic/Module/CrateCollide.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;

//-------------------------------------------------------------------------------------------------
class SabotageMilitaryFactoryCrateCollideModuleData : public CrateCollideModuleData
{
public:
	UnsignedInt m_sabotageFrames;

	SabotageMilitaryFactoryCrateCollideModuleData()
	{
		m_sabotageFrames = 0;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
    CrateCollideModuleData::buildFieldParse(p);

		static const FieldParse dataFieldParse[] =
		{
			{ "SabotageDuration", INI::parseDurationUnsignedInt, NULL, offsetof( SabotageMilitaryFactoryCrateCollideModuleData, m_sabotageFrames ) },
			{ 0, 0, 0, 0 }
		};
		p.add( dataFieldParse );
	}

};

//-------------------------------------------------------------------------------------------------
class SabotageMilitaryFactoryCrateCollide : public CrateCollide
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( SabotageMilitaryFactoryCrateCollide, "SabotageMilitaryFactoryCrateCollide" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SabotageMilitaryFactoryCrateCollide, SabotageMilitaryFactoryCrateCollideModuleData );

public:

	SabotageMilitaryFactoryCrateCollide( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

protected:

	/// This allows specific vetoes to certain types of crates and their data
	virtual Bool isValidToExecute( const Object *other ) const;

	/// This is the game logic execution function that all real CrateCollides will implement
	virtual Bool executeCrateBehavior( Object *other );

	virtual Bool isSabotageBuildingCrateCollide() const { return TRUE; }

};

#endif
