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

// FILE: FireWeaponUpdate.h /////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, August 2002
// Desc:   Update fires a weapon at its own feet as quickly as the weapon allows
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/FireWeaponUpdate.h"
#include "GameLogic/WeaponStatus.h"


//-------------------------------------------------------------------------------------------------
FireWeaponUpdateModuleData::FireWeaponUpdateModuleData()
{
	m_weaponTemplate = NULL;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void FireWeaponUpdateModuleData::buildFieldParse(MultiIniFieldParse& p) 
{
  UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] = 
	{
		{ "Weapon",	INI::parseWeaponTemplate,	NULL, offsetof( FireWeaponUpdateModuleData, m_weaponTemplate ) },
		{ 0, 0, 0, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireWeaponUpdate::FireWeaponUpdate( Thing *thing, const ModuleData* moduleData ) : 
	UpdateModule( thing, moduleData ),
	m_weapon(NULL)
{
	const WeaponTemplate *tmpl = getFireWeaponUpdateModuleData()->m_weaponTemplate;
	if (tmpl)
	{
		m_weapon = TheWeaponStore->allocateNewWeapon(tmpl, PRIMARY_WEAPON);
		m_weapon->loadAmmoNow( getObject() );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireWeaponUpdate::~FireWeaponUpdate( void )
{
	if (m_weapon)
		deleteInstance(m_weapon);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireWeaponUpdate::update( void )
{
	// If my weapon is ready, shoot it.
	if( isOkayToFire() )
	{
		m_weapon->forceFireWeapon( getObject(), getObject()->getPosition() );
	}
	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool FireWeaponUpdate::isOkayToFire()
{
	if( m_weapon == NULL )
		return FALSE;

	// Weapon is reloading
	if( m_weapon->getStatus() != READY_TO_FIRE )
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void FireWeaponUpdate::crc( Xfer *xfer )
{
	// extend base class
	UpdateModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void FireWeaponUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// weapon
	xfer->xferSnapshot( m_weapon );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void FireWeaponUpdate::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

}  // end loadPostProcess
