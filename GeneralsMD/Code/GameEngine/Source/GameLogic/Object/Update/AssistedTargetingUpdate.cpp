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

// FILE: AssistedTargetingUpdate.cpp /////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, September 2002
// Desc:   Outside influences can tell me to attack something out of my normal targeting range
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_WEAPONSLOTTYPE_NAMES
#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponSet.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/AssistedTargetingUpdate.h"
#include "GameLogic/Module/LaserUpdate.h"





//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AssistedTargetingUpdateModuleData::buildFieldParse(MultiIniFieldParse& p) 
{
  UpdateModuleData::buildFieldParse(p);
	static const FieldParse dataFieldParse[] = 
	{
		{ "AssistingClipSize",		INI::parseInt,		NULL, offsetof( AssistedTargetingUpdateModuleData, m_clipSize ) },
		{ "AssistingWeaponSlot",	INI::parseLookupList,	TheWeaponSlotTypeNamesLookupList, offsetof( AssistedTargetingUpdateModuleData, m_weaponSlot ) },
		{ "LaserFromAssisted",		INI::parseAsciiString,				NULL, offsetof( AssistedTargetingUpdateModuleData, m_laserFromAssistedName ) },
		{ "LaserToTarget",				INI::parseAsciiString,				NULL, offsetof( AssistedTargetingUpdateModuleData, m_laserToTargetName ) },
		{ 0, 0, 0, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
AssistedTargetingUpdate::AssistedTargetingUpdate( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_laserFromAssisted = NULL;
	m_laserToTarget = NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
AssistedTargetingUpdate::~AssistedTargetingUpdate( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool AssistedTargetingUpdate::isFreeToAssist() const
{
	// The reload times of my two weapons are tied together, so Ready is indicitive of either.
	const Object *me = getObject();
	if( !me->isAbleToAttack() )
		return FALSE;// This will cover under construction among other things

	Bool ready = me->getCurrentWeapon() && me->getCurrentWeapon()->getStatus() == READY_TO_FIRE;
	return ready;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AssistedTargetingUpdate::assistAttack( const Object *requestingObject, Object *victimObject )
{
	const AssistedTargetingUpdateModuleData *md = getAssistedTargetingUpdateModuleData();
	Object *me = getObject();
	if( !me->getAI() )
		return;

	// lock it just till the weapon is empty or the attack is "done"
	me->setWeaponLock( md->m_weaponSlot, LOCKED_TEMPORARILY );
	me->getAI()->aiAttackObject( victimObject, md->m_clipSize, CMD_FROM_AI );


	if( m_laserFromAssisted )
		makeFeedbackLaser( m_laserFromAssisted, requestingObject, me );
	if( m_laserToTarget )
		makeFeedbackLaser( m_laserToTarget, me, victimObject );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AssistedTargetingUpdate::makeFeedbackLaser( const ThingTemplate *laserTemplate, const Object *from, const Object *to )
{
	if( !getObject()->getControllingPlayer() )
		return;

	Team *laserTeam = getObject()->getControllingPlayer()->getDefaultTeam();
	Object *laser = TheThingFactory->newObject( laserTemplate, laserTeam );
	if( !laser )
		return;

	// Give it a good basis in reality to ensure it can draw when on screen.
	laser->setPosition(from->getPosition());
	
	Drawable *draw = laser->getDrawable();
	static const NameKeyType key_LaserUpdate = NAMEKEY( "LaserUpdate" );
	LaserUpdate *update = (LaserUpdate*)draw->findClientUpdateModule( key_LaserUpdate );
	if( !update )
	{
		TheGameLogic->destroyObject( laser );
		return;
	}

	update->initLaser( getObject(), to, from->getPosition(), to->getPosition(), "" );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime AssistedTargetingUpdate::update( void )
{

  const AssistedTargetingUpdateModuleData *d = getAssistedTargetingUpdateModuleData();

	m_laserFromAssisted = TheThingFactory->findTemplate( d->m_laserFromAssistedName );


	m_laserToTarget =TheThingFactory->findTemplate( d->m_laserFromAssistedName );


	return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AssistedTargetingUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void AssistedTargetingUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AssistedTargetingUpdate::loadPostProcess( void )
{
  const AssistedTargetingUpdateModuleData *d = getAssistedTargetingUpdateModuleData();

	m_laserFromAssisted = TheThingFactory->findTemplate( d->m_laserFromAssistedName );
	m_laserToTarget =TheThingFactory->findTemplate( d->m_laserFromAssistedName );

	// extend base class
	UpdateModule::loadPostProcess();

}  // end loadPostProcess
