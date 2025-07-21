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

// FILE: TunnelContain.cpp ////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:   A version of OpenContain that overrides where the passengers are stored: the Owning Player's
//					TunnelTracker.  All queries about capacity and contents are also redirected.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Player.h"
#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/TunnelTracker.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/OpenContain.h"
#include "GameLogic/Module/TunnelContain.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
TunnelContain::TunnelContain( Thing *thing, const ModuleData* moduleData ) : OpenContain( thing, moduleData )
{
	m_needToRunOnBuildComplete = true;
	m_isCurrentlyRegistered = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
TunnelContain::~TunnelContain()
{
}

void TunnelContain::addToContainList( Object *obj )
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	owningPlayer->getTunnelSystem()->addToContainList( obj );
}

//-------------------------------------------------------------------------------------------------
/** Remove 'obj' from the m_containList of objects in this module.
	* This will trigger an onRemoving event for the object that this module
	* is a part of and an onRemovedFrom event for the object being removed */
//-------------------------------------------------------------------------------------------------
void TunnelContain::removeFromContain( Object *obj, Bool exposeStealthUnits )
{

	// sanity
	if( obj == NULL )
		return;

	// trigger an onRemoving event for 'm_object' no longer containing 'itemToRemove->m_object'
	if( getObject()->getContain() )
	{
		getObject()->getContain()->onRemoving( obj );
	}
			
	// trigger an onRemovedFrom event for 'remove'
	obj->onRemovedFrom( getObject() );

	//
	// we can only remove this object from the contains list of this module if
	// it is actually contained by this module
	//
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return; //game tear down.  We do the onRemove* stuff first because this is allowed to fail but that still needs to be done

	owningPlayer->getTunnelSystem()->removeFromContain( obj, exposeStealthUnits );

}

//-------------------------------------------------------------------------------------------------
/** Remove all contained objects from the contained list */
//-------------------------------------------------------------------------------------------------
void TunnelContain::removeAllContained( Bool exposeStealthUnits )
{
	ContainedItemsList list;
	Player *owningPlayer = getObject()->getControllingPlayer();
	owningPlayer->getTunnelSystem()->swapContainedItemsList(list);

	ContainedItemsList::iterator it = list.begin();

	while ( it != list.end() )
	{
		Object *obj = *it++;
		DEBUG_ASSERTCRASH( obj, ("Contain list must not contain NULL element"));

		removeFromContain( obj, exposeStealthUnits );
	}
}

//-------------------------------------------------------------------------------------------------
/** Iterate the contained list and call the callback on each of the objects */
//-------------------------------------------------------------------------------------------------
void TunnelContain::iterateContained( ContainIterateFunc func, void *userData, Bool reverse )
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	owningPlayer->getTunnelSystem()->iterateContained( func, userData, reverse );
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onContaining( Object *obj )
{
	OpenContain::onContaining(obj);

	// objects inside a building are held
	obj->setDisabled( DISABLED_HELD );

}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onRemoving( Object *obj ) 
{
	OpenContain::onRemoving(obj);

	// object is no longer held inside a garrisoned building
	obj->clearDisabled( DISABLED_HELD );

	/// place the object in the world at position of the container m_object
	ThePartitionManager->registerObject( obj );
	obj->setPosition( getObject()->getPosition() );
	if( obj->getDrawable() )
	{
		obj->setSafeOcclusionFrame(TheGameLogic->getFrame()+obj->getTemplate()->getOcclusionDelay());
		obj->getDrawable()->setDrawableHidden( false );
	}

	doUnloadSound();
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onSelling()
{
	// A TunnelContain tells everyone to leave if this is the last tunnel
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;
	
	// We are the last tunnel, so kick everyone out.  This makes tunnels act like Palace and Bunker
	// rather than killing the occupants as if the last tunnel died.
	if( tunnelTracker->friend_getTunnelCount() == 1 )
		removeAllContained(FALSE);// Can't be order to exit, as I have no time to organize their exits.
	// If they don't go right now, I will delete them in a moment

	// Unregister after the kick out, or else the unregistering will activate a cavein-kill.
	// We need to do this in case someone sells their last two tunnels at the same time.
	if( m_isCurrentlyRegistered )
	{
		tunnelTracker->onTunnelDestroyed( getObject() );
		m_isCurrentlyRegistered = FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
Bool TunnelContain::isValidContainerFor(const Object* obj, Bool checkCapacity) const
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	return owningPlayer->getTunnelSystem()->isValidContainerFor( obj, checkCapacity );
}

UnsignedInt TunnelContain::getContainCount() const
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer && owningPlayer->getTunnelSystem() )
	{
		return owningPlayer->getTunnelSystem()->getContainCount();
	}
	return 0;
}

Int TunnelContain::getContainMax( void ) const 
{ 
	Player *owningPlayer = getObject()->getControllingPlayer();
	return owningPlayer->getTunnelSystem()->getContainMax();
}

const ContainedItemsList* TunnelContain::getContainedItemsList() const
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	return owningPlayer->getTunnelSystem()->getContainedItemsList();
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------------------------------
void TunnelContain::scatterToNearbyPosition(Object* obj)
{
	Object *theContainer = getObject();

	//
	// for now we will just set the position of the object that is being removed from us
	// at a random angle away from our center out some distance
	//
	
	//
	// pick an angle that is in the view of the current camera position so that
	// the thing will come out "toward" the player and they can see it
	// NOPE, can't do that ... all players screen angles will be different, unless
	// we maintain the angle of each players screen in the player structure or something
	//
	Real angle = GameLogicRandomValueReal( 0.0f, 2.0f * PI );
//	angle = TheTacticalView->getAngle();
//	angle -= GameLogicRandomValueReal( PI / 3.0f, 2.0f * (PI / 3.0F) );

	Real minRadius = theContainer->getGeometryInfo().getBoundingCircleRadius();
	Real maxRadius = minRadius + minRadius / 2.0f;
	const Coord3D *containerPos = theContainer->getPosition();
	Real dist = GameLogicRandomValueReal( minRadius, maxRadius );

	Coord3D pos;
	pos.x = dist * Cos( angle ) + containerPos->x;
	pos.y = dist * Sin( angle ) + containerPos->y;
	pos.z = TheTerrainLogic->getGroundHeight( pos.x, pos.y );

	// set orientation
	obj->setOrientation( angle );

	AIUpdateInterface *ai = obj->getAIUpdateInterface();
	if( ai )
	{
		// set position of the object at center of building and move them toward pos
		obj->setPosition( theContainer->getPosition() );
		ai->ignoreObstacle(theContainer);
 		ai->aiMoveToPosition( &pos, CMD_FROM_AI );

	}  // end if
	else
	{

		// no ai, just set position at the target pos
		obj->setPosition( &pos );

	}  // end else
}

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void TunnelContain::onDie( const DamageInfo * damageInfo )
{
	// override the onDie we inherit from OpenContain. no super call.
	if (!getTunnelContainModuleData()->m_dieMuxData.isDieApplicable(getObject(), damageInfo))
		return;

	if( !m_isCurrentlyRegistered )
		return;//it isn't registered as a tunnel

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	tunnelTracker->onTunnelDestroyed( getObject() );
	m_isCurrentlyRegistered = FALSE;
}  

//-------------------------------------------------------------------------------------------------
void TunnelContain::onDelete( void )
{
	// Being sold is a straight up delete.  no death

	if( !m_isCurrentlyRegistered )
		return;//it isn't registered as a tunnel

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	tunnelTracker->onTunnelDestroyed( getObject() );
	m_isCurrentlyRegistered = FALSE;
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onCreate( void )
{
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onBuildComplete( void )
{
	if( ! shouldDoOnBuildComplete() )
		return;

	m_needToRunOnBuildComplete = false;

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	tunnelTracker->onTunnelCreated( getObject() );
	m_isCurrentlyRegistered = TRUE;
} 

// ------------------------------------------------------------------------------------------------
/** Per frame update */
// ------------------------------------------------------------------------------------------------
UpdateSleepTime TunnelContain::update( void )
{
	// extending functionality to heal the units within the tunnel system
	OpenContain::update();
	const TunnelContainModuleData *modData = getTunnelContainModuleData();
	
	Object *obj = getObject();
	Player *controllingPlayer = NULL;
	if (obj)
	{
		controllingPlayer = obj->getControllingPlayer();
	}
	if (controllingPlayer)
	{
		TunnelTracker *tunnelSystem = controllingPlayer->getTunnelSystem();
		if (tunnelSystem)
		{
			tunnelSystem->healObjects(modData->m_framesForFullHeal);
		}

		// check for attacked.
		BodyModuleInterface *body = obj->getBodyModule();
		if (body) {
			const DamageInfo *info = body->getLastDamageInfo();
			if (info) {
				if (body->getLastDamageTimestamp() + LOGICFRAMES_PER_SECOND > TheGameLogic->getFrame()) {
					// winner.
					ObjectID attackerID = info->in.m_sourceID;
					Object *attacker = TheGameLogic->findObjectByID(attackerID);
					if( attacker )
					{
						if (obj->getRelationship(attacker) == ENEMIES) {
							tunnelSystem->updateNemesis(attacker);
						}
					}
				}
			}
		}
	}
	return UPDATE_SLEEP_NONE;

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void TunnelContain::crc( Xfer *xfer )
{

	// extend base class
	OpenContain::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void TunnelContain::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	OpenContain::xfer( xfer );

	// need to run on build complete
	xfer->xferBool( &m_needToRunOnBuildComplete );

	// Currently registered with owning player
	xfer->xferBool( &m_isCurrentlyRegistered );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void TunnelContain::loadPostProcess( void )
{

	// extend base class
	OpenContain::loadPostProcess();

}  // end loadPostProcess
