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

// FILE: GarrisonContain.cpp //////////////////////////////////////////////////////////////////////
// Author: Colin Day, February 2002
// Desc:   Contain module for structures that can be garrisoned
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/GameState.h"
#include "Common/PerfTimer.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/RandomValue.h"
#include "Common/Team.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameLogic/AIPathfind.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/GarrisonContain.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Weapon.h"

#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/InGameUI.h"
#include "GameClient/View.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
enum { MUZZLE_FLASH_LIFETIME = LOGICFRAMES_PER_SECOND / 7 };

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
GarrisonContainModuleData::GarrisonContainModuleData( void )
{

	//
	// by default we say that transports can have infantry inside them, this will be totally
	// overwritten by any data provided from the INI entry tho
	//
	m_allowInsideKindOf = MAKE_KINDOF_MASK( KINDOF_INFANTRY );

	m_mobileGarrison = FALSE;
	m_doIHealObjects = false;			///< if T, then I heal objects that are inside of me
	m_framesForFullHeal = 1.0f;		///< the number of frames something inside of me takes to heal
	m_immuneToClearBuildingAttacks = false;

	m_initialRoster.count = 0;
}  // end if

//-----------------------------------------------------------------------------
inline Real calcDistSqr(const Coord3D& a, const Coord3D& b)
{
	return sqr(a.x - b.x) + sqr(a.y - b.y) + sqr(a.z - b.z);
}

// ------------------------------------------------------------------------------------------------
/** Given the target position, find the garrison point that is closest to it */
// ------------------------------------------------------------------------------------------------
Int GarrisonContain::findClosestFreeGarrisonPointIndex( Int conditionIndex,
																												const Coord3D *targetPos )
{
// TheSuperHackers @info helmutbuhler 05/05/2025 This debug mutates the code to become CRC incompatible
#if defined(RTS_DEBUG) || !RETAIL_COMPATIBLE_CRC
	DEBUG_ASSERTCRASH(m_garrisonPointsInitialized, ("garrisonPoints are not inited"));
#endif

	// sanity
	if( targetPos == NULL || m_garrisonPointsInUse == MAX_GARRISON_POINTS )
		return GARRISON_INDEX_INVALID;

	Int closestIndex = GARRISON_INDEX_INVALID;
	Real closestDistSq = -1.0f;
	Real distSq;
	for( Int i = 0; i < MAX_GARRISON_POINTS; ++i )
	{

		// only consider free garrison points
		if( m_garrisonPointData[ i ].object == NULL )
		{

			// compute the squared distance between these two points
			distSq = calcDistSqr(*targetPos, m_garrisonPoint[ conditionIndex ][ i ]);

			if( distSq < closestDistSq || closestDistSq == -1.0f )
			{

				closestDistSq = distSq;
				closestIndex = i;

			}  // end if

		}  // end if

	}  // end for i

	return closestIndex;

}  // end findClosestFreeGarrisonPointIndex

// ------------------------------------------------------------------------------------------------
/** Given the object, return the garrison point index the object is placed at ... if any */
// ------------------------------------------------------------------------------------------------
Int GarrisonContain::getObjectGarrisonPointIndex( Object *obj )
{

	// sanity
	if( obj == NULL )
		return GARRISON_INDEX_INVALID;

	for( Int i = 0; i < MAX_GARRISON_POINTS; ++i )
		if( m_garrisonPointData[ i ].object == obj )
			return i;

	return GARRISON_INDEX_INVALID;

}  // end getObjectGarrisonPointIndex

// ------------------------------------------------------------------------------------------------
/** Put the object at the specified garrison point by index */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::putObjectAtGarrisonPoint( Object *obj,
																								ObjectID targetID,
																								Int conditionIndex,
																								Int pointIndex )
{
	DEBUG_ASSERTCRASH(m_garrisonPointsInitialized, ("garrisonPoints are not inited"));

	// sanity
	if( obj == NULL || pointIndex < 0 || pointIndex >= MAX_GARRISON_POINTS ||
			conditionIndex < 0 || conditionIndex >= MAX_GARRISON_POINT_CONDITIONS )
	{

		DEBUG_CRASH(( "GarrisionContain::putObjectAtGarrisionPoint - Invalid arguments" ));
		return;

	}  // end if

	// make sure this point is empty
	if( m_garrisonPointData[ pointIndex ].object != NULL )
	{

		DEBUG_CRASH(( "GarrisonContain::putObjectAtGarrisonPoint - Garrison Point '%d' is not empty",
									pointIndex ));
		return;

	}  // end if

	// get the position we're going to use
	Coord3D pos = m_garrisonPoint[ conditionIndex ][ pointIndex ];

	// set the object position
	obj->setPosition( &pos );

	// save the data for being place at this point
	m_garrisonPointData[ pointIndex	].object = obj;
	m_garrisonPointData[ pointIndex	].targetID = targetID;
	m_garrisonPointData[ pointIndex	].placeFrame = TheGameLogic->getFrame();
	++m_garrisonPointsInUse;

	//
	// create a drawable that has a gun barrel which will show there is an object at this
	// garrison point ready to shoot
	//
	static const ThingTemplate *muzzle = TheThingFactory->findTemplate( "GarrisonGun" );
	DEBUG_ASSERTCRASH( muzzle, ("Warning, Object 'GarrisonGun' not found and is need for Garrison gun effects") );
	if( muzzle )
	{
		Drawable *draw = TheThingFactory->newDrawable( muzzle );
		if( draw )
		{

			// set position of the drawable at the garrison fire point
			draw->setPosition( &pos );

			// record the drawable in our data array
			m_garrisonPointData[ pointIndex ].effect = draw;
			m_garrisonPointData[ pointIndex ].lastEffectFrame = 0;

			//Copy shroud status from our container.
			Drawable *containerDrawable=getObject()->getDrawable();
			if (containerDrawable)
				draw->setFullyObscuredByShroud(containerDrawable->getFullyObscuredByShroud());

		}  // end if

	}  // end if

/*
UnicodeString msg;
msg.format( L"Added object '%S'(%d) to point '%d'",
						obj->getTemplate()->getName().str(),
						obj->getID(),
						pointIndex );
TheInGameUI->message( msg );
*/

}  // end putObjectAtGarrisonPoint

// ------------------------------------------------------------------------------------------------
/** Given the current state of the structure, return the condition index we are to use
	* from the garrison point position arrays */
// ------------------------------------------------------------------------------------------------
Int GarrisonContain::findConditionIndex( void )
{
	BodyModuleInterface *body = getObject()->getBodyModule();
	BodyDamageType bodyDamage = body->getDamageState();
	Int index = GARRISON_INDEX_INVALID;

	switch( bodyDamage )
	{

		// --------------------------------------------------------------------------------------------
		case BODY_PRISTINE:

			index = GARRISON_POINT_PRISTINE;
			break;

		// --------------------------------------------------------------------------------------------
		case BODY_DAMAGED:

			index = GARRISON_POINT_DAMAGED;
			break;

		// --------------------------------------------------------------------------------------------
		case BODY_REALLYDAMAGED:
		case BODY_RUBBLE:

			index = GARRISON_POINT_REALLY_DAMAGED;
			break;

		// --------------------------------------------------------------------------------------------
		default:

			DEBUG_CRASH(( "GarrisonContain::findConditionIndex - Unknown body damage type '%d'",
										bodyDamage ));
			break;

	}  // end switch

	return index;

}  // end findConditionIndex

//-------------------------------------------------------------------------------------------------
//The weapon system would like to perform a range check assuming the object is placed in the
//best possible available garrison position. If found, we change sourcePos to that position.
//-------------------------------------------------------------------------------------------------
Bool GarrisonContain::calcBestGarrisonPosition( Coord3D *sourcePos, const Coord3D *targetPos )
{
	// sanity
	if( !sourcePos || !targetPos )
		return FALSE;

	// find which garrison point position array we will used based on body condition
	Int conditionIndex = findConditionIndex();

	// get the index of the garrison point that is closest to the target position
	Int placeIndex = findClosestFreeGarrisonPointIndex( conditionIndex, targetPos );
	if( placeIndex == GARRISON_INDEX_INVALID )
	{
		DEBUG_CRASH( ("GarrisonContain::calcBestGarrisonPosition - Unable to find suitable garrison point.") );
		return FALSE;
	}

	sourcePos->set( &(m_garrisonPoint[ conditionIndex ][ placeIndex ]) );
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//The AI is entering the aim state and would like to move the unit to the best position, perform
//a range check, and if it succeeds, leave him there -- otherwise, remove him immediately.
//-------------------------------------------------------------------------------------------------
Bool GarrisonContain::attemptBestFirePointPosition( Object *source, Weapon *weapon, Object *victim )
{
	//Sanity
	if( !source || !victim || !weapon )
	{
		return FALSE;
	}

	//If this object is already at a garrison point, remove him.
	Int existingIndex = getObjectGarrisonPointIndex( source );
	if( existingIndex != GARRISON_INDEX_INVALID )
	{
		removeObjectFromGarrisonPoint( source, existingIndex );
	}

	putObjectAtBestGarrisonPoint( source, victim, NULL );

	//Okay, now we have positioned the object in the best position for the victim.
	//Now check if we are able to fire on our victim.
	if( weapon->isWithinAttackRange( source, victim ) )
	{
		return TRUE;
	}

	//Crap, we failed... so remove the object from the garrison point.
	existingIndex = getObjectGarrisonPointIndex( source );
	if( existingIndex != GARRISON_INDEX_INVALID )
	{
		removeObjectFromGarrisonPoint( source, existingIndex );
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//The AI is entering the aim state and would like to move the unit to the best position, perform
//a range check, and if it succeeds, leave him there -- otherwise, remove him immediately.
//-------------------------------------------------------------------------------------------------
Bool GarrisonContain::attemptBestFirePointPosition( Object *source, Weapon *weapon, const Coord3D *targetPos )
{
	//Sanity
	if( !source || !targetPos || !weapon )
	{
		return FALSE;
	}

	//If this object is already at a garrison point, remove him.
	Int existingIndex = getObjectGarrisonPointIndex( source );
	if( existingIndex != GARRISON_INDEX_INVALID )
	{
		removeObjectFromGarrisonPoint( source, existingIndex );
	}

	putObjectAtBestGarrisonPoint( source, NULL, targetPos );

	//Okay, now we have positioned the object in the best position for the targetPos.
	//Now check if we are able to fire on our targetPos.
	if( weapon->isWithinAttackRange( source, targetPos ) )
	{
		return TRUE;
	}

	//Crap, we failed... so remove the object from the garrison point.
	existingIndex = getObjectGarrisonPointIndex( source );
	if( existingIndex != GARRISON_INDEX_INVALID )
	{
		removeObjectFromGarrisonPoint( source, existingIndex );
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
/** Place the object at the "best" garrison point position so it's on the same "side" of
	* the structure that its target is */
//-------------------------------------------------------------------------------------------------
void GarrisonContain::putObjectAtBestGarrisonPoint( Object *obj, Object *target, const Coord3D *targetPos )
{

	// sanity
	if( obj == NULL || (target == NULL && targetPos == NULL) )
		return;

	// if obj target, override pos
	if (target != NULL)
		targetPos = target->getPosition();

	// if this object is already at a garrison point do nothing
	if( getObjectGarrisonPointIndex( obj ) != GARRISON_INDEX_INVALID )
		return;

	// find which garrison point position array we will used based on body condition
	Int conditionIndex = findConditionIndex();

	// get the index of the garrison point that is closest to the target position
	Int placeIndex = findClosestFreeGarrisonPointIndex( conditionIndex, targetPos );
	DEBUG_ASSERTCRASH( placeIndex != GARRISON_INDEX_INVALID,
										 ("GarrisonContain::putObjectAtBestGarrisonPoint - Unable to find suitable garrison point for '%s'",
										 obj->getTemplate()->getName().str()) );

	// put it here
	putObjectAtGarrisonPoint( obj, target ? target->getID() : INVALID_ID, conditionIndex, placeIndex );

}  // end putObjectAtBestGarrisonPoint

// ------------------------------------------------------------------------------------------------
/** Remove the object from the garrison point position and replace at the center of the building */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::removeObjectFromGarrisonPoint( Object *obj, Int index )
{

	// sanity
	if( obj == NULL )
		return;

	// search for the object in the garrison point data, if found, remove it
	Int removeIndex = index;
	if( removeIndex == SEARCH_FOR_REMOVE )
	{

		for( Int i = 0; i < MAX_GARRISON_POINTS; ++i )
		{

			if( m_garrisonPointData[ i ].object == obj )
			{

				removeIndex = i;
				break;

			}  // end if

		}  // end for i

	}  // end if

	// validate the index slot to remove
	if( removeIndex < 0 || removeIndex >= MAX_GARRISON_POINTS )
	{

		//
		// this is not an error, if a search was ordered, we may very well not find the
		// object at a garrison point and therefore can't remove it
		//
		return;

	}  // end if

	// remove from this spot
	m_garrisonPointData[ removeIndex ].object = NULL;
	m_garrisonPointData[ removeIndex ].targetID = INVALID_ID;
	m_garrisonPointData[ removeIndex ].placeFrame = 0;
	m_garrisonPointData[ removeIndex ].lastEffectFrame = 0;
	--m_garrisonPointsInUse;

	// destroy drawable for gun barrel and effects if present
	if( m_garrisonPointData[ removeIndex ].effect )
		TheGameClient->destroyDrawable( m_garrisonPointData[ removeIndex ].effect );
	m_garrisonPointData[ removeIndex ].effect = NULL;

	// set the position of the object to back to the center of the garrisoned building
	obj->setPosition( getObject()->getPosition() );

/*
UnicodeString msg;
msg.format( L"Removed object '%S'(%d) from point '%d'",
						obj->getTemplate()->getName().str(),
						obj->getID(),
						removeIndex );
TheInGameUI->message( msg );
*/

}  // end removeObjectFromGarrisonPoint

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
GarrisonContain::GarrisonContain( Thing *thing, const ModuleData *moduleData ) :
								 OpenContain( thing, moduleData )
{
	Int i, j;

	m_originalTeam = NULL;
	m_hideGarrisonedStateFromNonallies = FALSE;
	m_garrisonPointsInUse = 0;
	m_garrisonPointsInitialized = FALSE;

	for( i = 0; i < MAX_GARRISON_POINTS; i++ )
	{

		m_garrisonPointData[ i ].object = NULL;
		m_garrisonPointData[ i ].targetID = INVALID_ID;
		m_garrisonPointData[ i ].placeFrame = 0;
		m_garrisonPointData[ i ].lastEffectFrame = 0;
		m_garrisonPointData[ i ].effect = NULL;

		for( j = 0; j < MAX_GARRISON_POINT_CONDITIONS; ++j )
			m_garrisonPoint[ j ][ i ].zero();

	}  // end for i

	m_rallyValid = FALSE;
	m_exitRallyPoint.zero();

}  // end GarrisonContain

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
GarrisonContain::~GarrisonContain( void )
{

}  // end ~GarrisonContain

//-------------------------------------------------------------------------------------------------
/** can this container contain this kind of object?
	and, if checkCapacity is TRUE, does this container have enough space
	left to hold the given unit? */
// ------------------------------------------------------------------------------------------------
Bool GarrisonContain::isValidContainerFor(const Object* obj, Bool checkCapacity) const
{

	// extend functionality // this just tests kindof masks
	if( OpenContain::isValidContainerFor( obj, checkCapacity ) == false )
		return false;

	// zero-health buildings are not garrisonable.
	if (getObject()->getBodyModule()->getHealth() <= 0.0f)
		return false;

	// ReallyDamaged buildings are not garrisonable as well.
	if( getObject()->getBodyModule()->getDamageState() == BODY_REALLYDAMAGED && !getObject()->isKindOf( KINDOF_GARRISONABLE_UNTIL_DESTROYED ) )
		return false;

	if (obj && !obj->isKindOf(KINDOF_NO_GARRISON))
	{
		if (checkCapacity)
		{
			Int garrisonMax = getContainMax();
			Int containCount = getContainCount();
			return ( containCount < garrisonMax );
		}
		else
		{
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
/** Any objects that are sitting at the garrison points which no longer have targets need
	* to be moved to the center of the building and taken off the garrison point */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::removeInvalidObjectsFromGarrisonPoints( void )
{
	Object *obj;

	if (m_garrisonPointsInUse == 0)
		return;	// my, that was easy

	for( Int i = 0; i < MAX_GARRISON_POINTS; ++i )
	{

		obj = m_garrisonPointData[ i ].object;
		if( obj )
		{
			AIUpdateInterface *ai = obj->getAIUpdateInterface();

			Bool targetIsValid = true;	// assume true for now...
			Object *goalObject = ai->getGoalObject();
			if( goalObject )
			{
				Weapon *weapon = obj->getCurrentWeapon();
				if( !weapon || !weapon->isWithinAttackRange( obj, goalObject ) )
				{
					//As a garrisoned member, if our target is out of range,
					//then get out of the space, because someone else might
					//be able to shoot it.
					targetIsValid = false;
				}
			}

			// note that we can be attacking a position, rather than an object...
			if( !obj->testStatus(OBJECT_STATUS_IS_ATTACKING) || !targetIsValid )
			{
				removeObjectFromGarrisonPoint( obj, i );
			}

		}  // end if

	}  // end for i

}  // end removeInvalidObjectsFromGarrisonPoints

// ------------------------------------------------------------------------------------------------
/** Are there any objects in the center that have now obtained targets and need to move to
	* a garrison point */
	// ------------------------------------------------------------------------------------------------
void GarrisonContain::addValidObjectsToGarrisonPoints( void )
{
	const ContainedItemsList& containList = getContainList();

	if (containList.empty())
		return;

	for( ContainedItemsList::const_iterator it = containList.begin(); it != containList.end(); ++it )
	{
		Object* obj = *it;
		AIUpdateInterface* ai = obj->getAIUpdateInterface();
		if( ai )
		{
			Object *victim = ai->getCurrentVictim();
			const Coord3D *victimPos = ai->getCurrentVictimPos();

			//
			// add this object to the garrison point that is closest to its target if it's not
			// already in there
			//
			if( victim )
				putObjectAtBestGarrisonPoint( obj, victim, NULL );
			else if( victimPos )
				putObjectAtBestGarrisonPoint( obj, NULL, victimPos );

		}  // end if

	}  // end for it

}  // end addValidObjectsToGarrisonPoints

// ------------------------------------------------------------------------------------------------
/** Every frame this method is called.  It keeps any of the attacking units at any of the
	* fire points closest to their active target and shuffles them around to any open garrison
	* points that are available if they are closer.  We will also track our targets position
	* and orient any effect stuff we need to (gun barrel / muzzle flash) */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::trackTargets( void )
{
	Int conditionIndex = findConditionIndex();
	const ContainedItemsList& containList = getContainList();
	AIUpdateInterface *ai;
	Object *obj;

	DEBUG_ASSERTCRASH(m_garrisonPointsInitialized || containList.empty(), ("garrisonPoints are not inited"));

	for( ContainedItemsList::const_iterator it = containList.begin(); it != containList.end(); ++it )
	{
		// get the object
		obj = *it;

		// only consider objects that are actually at garrison points for re-shuffling
		Int ourIndex = getObjectGarrisonPointIndex( obj );
		if( ourIndex != GARRISON_INDEX_INVALID )
		{

			// does this object have a target?
			ai = obj->getAIUpdateInterface();
			if( ai )
			{
				Object *victim = ai->getCurrentVictim();
				// even though the target position can't change in some cases, still must do this code at least once.
				const Coord3D *victimPos = ai->getCurrentVictimPos();

				if( victim || victimPos )
				{
					if (victim)
						victimPos = victim->getPosition();
					const Coord3D *ourPos = obj->getPosition();

					// find the closest free (of all remaining) garrison points to our target
					Int newIndex = findClosestFreeGarrisonPointIndex( conditionIndex,
																														victimPos );

					// if unable to find another garrison point, don't bother
					if( newIndex != GARRISON_INDEX_INVALID )
					{

						// get the distance from our current index to the target
						Real currentDistSq = calcDistSqr(*victimPos, *ourPos );

						// get the distance from the newly chosen index
						Real newDistSq = calcDistSqr(*victimPos, m_garrisonPoint[ conditionIndex ][ newIndex ] );

						// if the newly chosen index is closer than our current index, switch
						if( newDistSq < currentDistSq )
						{

							// remove from the old index
							removeObjectFromGarrisonPoint( obj, ourIndex );

							// place at the new index
							putObjectAtGarrisonPoint( obj, victim ? victim->getID() : INVALID_ID, conditionIndex, newIndex );

						}  // end if, new index is closer

					}  // end if, possible closer index was found

					//
					// we are now either at a new garrison fire point, or we have remained at our
					// existing point still tracking our target.  Orient the effect drawable which
					// shows the gun barrel and muzzle flash towards our target position
					//
					if( m_garrisonPointData[ ourIndex ].effect )
					{
						Coord2D v;
						v.x = victimPos->x - ourPos->x;
						v.y = victimPos->y - ourPos->y;
//					v.z = victomPos->z - ourPos.z;

						// orient the effect object towards the victim position
						m_garrisonPointData[ ourIndex ].effect->setOrientation( v.toAngle() );

					}  // end if

				}  // end if, victim present

			}  // end if, ai

		}  // end if, we're at a garrison point

	}  // end for it

}  // end trackTargets

// ------------------------------------------------------------------------------------------------
/** Remove all the objects at garrison points back to the center and redeploy them among the
	* garrison points.  NOTE that we are preserving the frame in which the object was put
	* at the garrison point originally as this method is used when the model condition changes
	* which could shuffle the garrison point positions but that shouldn't logically change
	* when an object was placed at the point */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::redeployOccupants( void )
{
	GarrisonPointData garrisonPointDataCopy[ MAX_GARRISON_POINTS ];
	Int i;

	// copy the current set of garrison point data sets
	for( i = 0; i < MAX_GARRISON_POINTS; ++i )
		garrisonPointDataCopy[ i ] = m_garrisonPointData[ i ];

	// remove the occupants
	removeInvalidObjectsFromGarrisonPoints();

	// redeploy them
	addValidObjectsToGarrisonPoints();

	// restore the frame markers that things were recorded as entering their point
	Int index;
	for( i = 0; i < MAX_GARRISON_POINTS; ++i )
	{

		if( garrisonPointDataCopy[ i ].object )
		{

			// where was this object redeployed
			index = getObjectGarrisonPointIndex( garrisonPointDataCopy[ i ].object );
			if( index != GARRISON_INDEX_INVALID )
				m_garrisonPointData[ index ].placeFrame = garrisonPointDataCopy[ i ].placeFrame;

		}  // end if

	}  // end for i

}  // end redeployOccupants

// ------------------------------------------------------------------------------------------------
/** Do any effects during an update cycle that we need to */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::updateEffects( void )
{
	UnsignedInt currentFrame = TheGameLogic->getFrame();
	const ContainedItemsList& containList = getContainList();

	for( ContainedItemsList::const_iterator it = containList.begin(); it != containList.end(); ++it )
	{
		Object *obj;

		// get the object
		obj = *it;

		//
		// did the object fire last frame, if so make a muzzle flash if needed at the
		// garrison point
		//
		if( obj->getLastShotFiredFrame() == currentFrame - 1 )
		{
			Int garrisonIndex = getObjectGarrisonPointIndex( obj );

			// only consider doing muzzle flash logic if the object is actually at a garrison point
			if( garrisonIndex != GARRISON_INDEX_INVALID )
			{

				// set the model condition for the effect object to show the muzzle flash
				Drawable *effect = m_garrisonPointData[ garrisonIndex ].effect;
				if( effect )
				{

					// set the model condition
					effect->setModelConditionState( MODELCONDITION_FIRING_A );

					// mark this "fire frame" so we can turn it off in a little while
					m_garrisonPointData[ garrisonIndex ].lastEffectFrame = currentFrame;

				}  // end if

			}  // end if, object is at garrision point

		}  // end if, object shot last frame

	}  // end for containment iterator

	// remove any firing effects for time that has passed
	for( Int i = 0; i < MAX_GARRISON_POINTS; ++i )
	{

		if( m_garrisonPointData[ i ].effect &&
				m_garrisonPointData[ i ].lastEffectFrame != 0 &&
				currentFrame - m_garrisonPointData[ i ].lastEffectFrame > MUZZLE_FLASH_LIFETIME  )
		{

			// clear the model condition
			m_garrisonPointData[ i ].effect->clearModelConditionState( MODELCONDITION_FIRING_A );

			// clear the last effect frame
			m_garrisonPointData[ i ].lastEffectFrame = 0;

		}  // end if

	}  // end for i

}  // end updateEffects

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime GarrisonContain::update( void )
{
	const GarrisonContainModuleData *modData = getGarrisonContainModuleData();

	// extend functionality
	UpdateSleepTime result;
	result = OpenContain::update();

	// remove effectively dead objects from this garrison container
	const ContainedItemsList& containList = getContainList();
	Object *contained;
	for( ContainedItemsList::const_iterator it = containList.begin(); it != containList.end(); /*empty*/ )
	{

		// get object
		contained = *it;

		// increment iterator, we may delete the object
		++it;

		// remove if dead
		if( contained->isEffectivelyDead() )
		{

			// remove from container
			removeFromContain( contained );

			// set the safe occlusion frame to way way way in the future so we never see it during death
			#define HUGE_FRAME_IN_FUTURE (LOGICFRAMES_PER_SECOND * 1000)
			contained->setSafeOcclusionFrame( TheGameLogic->getFrame() + HUGE_FRAME_IN_FUTURE );

		}  // end if

	}  // end for, it

	// are there any objects at the garrison points who now need to go back to the center of the structure
	removeInvalidObjectsFromGarrisonPoints();

	//
	// are there any objects in the center that have now obtained targets and need to move to
	// a garrison point
	//
	addValidObjectsToGarrisonPoints();

	// any units that have just fired need to have a muzzle flash display out of the fire point
	updateEffects();

	//
	// given all the objects that are at the garrison points shooting at something, if their
	// target moves around the structure and closer to another open garrison point we want
	// to shuffle our object to the new closest garrison point.  We'll also track the target
	// here and set orientation for any effects we need to
	//
	trackTargets();

	healObjects();

	if (modData->m_mobileGarrison && (getObject()->isMobile() == TRUE) )
	{
		moveObjectsWithMe();
	}
	else
	{
		// sanity information
		DEBUG_ASSERTCRASH( getObject()->isMobile() == FALSE,
		 ("GarrisonContain::update - Objects with garrison contain can be spec'd as 'mobile' in the INI. Do you really want to do this?") );
	}

	return UPDATE_SLEEP_NONE;
}  // end update

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GarrisonContain::healObjects( void )
{
	const GarrisonContainModuleData *modData = getGarrisonContainModuleData();

	if (!modData->m_doIHealObjects)
		return;

	const ContainedItemsList& containList = getContainList();
	for( ContainedItemsList::const_iterator it = containList.begin(); it != containList.end(); ++it )
	{
		Object *obj;

		// get the object
		obj = *it;

		healSingleObject(obj, modData->m_framesForFullHeal);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GarrisonContain::healSingleObject( Object *obj, Real framesForFullHeal)
{
	// setup the healing damageInfo structure with all but the amount
	DamageInfo healInfo;
	healInfo.in.m_damageType = DAMAGE_HEALING;
	healInfo.in.m_deathType = DEATH_NONE;
	//healInfo.in.m_sourceID = getObject()->getID();

	// get body module of the thing to heal
	BodyModuleInterface *body = obj->getBodyModule();

	// if we've been in here long enough ... set our health to max
	if( TheGameLogic->getFrame() - obj->getContainedByFrame() >= framesForFullHeal )
	{

		// set the amount to max just to be sure we're at the top
		healInfo.in.m_amount = body->getMaxHealth();

		// set max health
		body->attemptHealing( &healInfo );

	}  // end if
	else
	{
		//
		// given the *whole* time it would take to heal this object, lets pretend that the
		// object is at zero health ... and give it a sliver of health as if it were at 0 health
		// and would be fully healed at 'framesForFullHeal'
		//
		healInfo.in.m_amount = body->getMaxHealth() / framesForFullHeal;

		// do the healing
		body->attemptHealing( &healInfo );

	}  // end else
}

//-------------------------------------------------------------------------------------------------
/** return the player that *appears* to control this unit. if null,
		use getObject()->getControllingPlayer() instead. */
// ------------------------------------------------------------------------------------------------
const Player* GarrisonContain::getApparentControllingPlayer( const Player* observingPlayer ) const
{
	const Player* myPlayer = getObject()->getControllingPlayer();

	if ( m_hideGarrisonedStateFromNonallies && m_originalTeam && myPlayer && observingPlayer )
	{
		Relationship r = myPlayer->getRelationship(observingPlayer->getDefaultTeam());
		if (r != ALLIES)
			return m_originalTeam->getControllingPlayer();
	}
	return myPlayer;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GarrisonContain::recalcApparentControllingPlayer( void )
{
	//Record original team first time through.
	if( m_originalTeam == NULL )
	{
		m_originalTeam = getObject()->getTeam();
	}

	// (hokey trick: if our team is null, nuke originalTeam -- this
	// usually means we are being called during game-teardown and
	// the teams are no longer valid...)
	if (getObject()->getTeam() == NULL)
		m_originalTeam = NULL;
	// Check to see if we have any units contained in our object
	if( getContainCount() > 0 )
	{
		ContainedItemsList::const_iterator it = getContainList().begin();
		Object *rider = *it;

		// Check to see if all the contained units are stealthy.  Need to set this flag before the capture,
		// since the Radar refresh in setTeam will want to use it to decide our color.
		Bool detected = rider->getStatusBits().test( OBJECT_STATUS_DETECTED );
		m_hideGarrisonedStateFromNonallies = ( !detected && ( getStealthUnitsContained() == getContainCount() ) );

		Player* controller = rider->getControllingPlayer();
		Team *team = controller ? controller->getDefaultTeam() : NULL;
		if( team )
		{
			getObject()->setTeam( team );
		}
	}
	else
	{
		//Nothing in object, so set team to original team.
		getObject()->setTeam( m_originalTeam );
		m_hideGarrisonedStateFromNonallies = false;
	}

	//Only allow the garrison state to be set if the client team knows that it is garrisoned.
	Drawable *draw = getObject()->getDrawable();
	if( draw )
	{
		Bool setModelGarrisoned = FALSE;


		if ( getContainCount() > 0 )
		{
			ContainedItemsList::const_iterator it = getContainList().begin();
			Object *occupant = *it;
			Bool detected = occupant->getStatusBits().test( OBJECT_STATUS_DETECTED );

			if( detected || (getApparentControllingPlayer(ThePlayerList->getLocalPlayer()) == getObject()->getControllingPlayer()) )
			{
				setModelGarrisoned = TRUE;
			}

		}


		if ( setModelGarrisoned )
			draw->setModelConditionState( MODELCONDITION_GARRISONED );
		else
			draw->clearModelConditionState( MODELCONDITION_GARRISONED );


		// Handle the team color that is rendered
		const Player* controller = getApparentControllingPlayer(ThePlayerList->getLocalPlayer());
		if (controller)
		{
			if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
				draw->setIndicatorColor( controller->getPlayerNightColor() );
			else
				draw->setIndicatorColor( controller->getPlayerColor() );
		}

		// now that we have an object inside us, we need to get all the garrison point positions
		// if we don't already have them.
		if( getContainCount() > 0 && m_garrisonPointsInitialized == FALSE )
		{
			loadGarrisonPoints();
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** Load the garrison point position data and save for use later */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::loadGarrisonPoints( void )
{
	const GarrisonContainModuleData *modData = getGarrisonContainModuleData();

	Object *structure = getObject();
	Int i, j;
	Bool gBonesFound = FALSE;

	//
	// initialize all the garrison points to the center of the object, this assumes that we
	// will never move (these points are cached)
	//
	for( i = 0; i < MAX_GARRISON_POINT_CONDITIONS; ++i )
		for( j = 0; j < MAX_GARRISON_POINTS; ++j )
			m_garrisonPoint[ i ][ j ] = *(structure->getPosition());

	//
	// in order to get all the garrison point positions we will actually switch the model
	// condition through the garrisoned pristine, damaged, and really damaged states.  This
	// is especially important because at the time or "garrisoning" a model condition may
	// not actually switch to a garrisoned state because of units that garrison things
	// all stealthy.  By caching all the bone positions once we will have access to
	// any of the points for any condition state at any time
	//
	{
		Int conditionIndex;
		Int count = 0;

		// save the original paramters for the model condition
		Drawable* draw = structure->getDrawable();
		const ModelConditionFlags originalFlags = draw->getModelConditionFlags();
		ModelConditionFlags clearFlags;
		ModelConditionFlags setFlags;

		// pristine garrisoned
		clearFlags.clear();
		setFlags.clear();
		clearFlags.set( MODELCONDITION_REALLY_DAMAGED );
		clearFlags.set( MODELCONDITION_RUBBLE );
		clearFlags.set( MODELCONDITION_SPECIAL_DAMAGED );
		clearFlags.set( MODELCONDITION_DAMAGED );
		setFlags.set( MODELCONDITION_GARRISONED );
		structure->clearAndSetModelConditionFlags( clearFlags, setFlags );
		conditionIndex = GARRISON_POINT_PRISTINE;

		count = structure->getMultiLogicalBonePosition("FIREPOINT", MAX_GARRISON_POINTS, m_garrisonPoint[ conditionIndex ], NULL);

		if ( count > 0) gBonesFound = TRUE;

		// damaged garrisoned
		clearFlags.clear();
		setFlags.clear();
		clearFlags.set( MODELCONDITION_REALLY_DAMAGED );
		clearFlags.set( MODELCONDITION_RUBBLE );
		clearFlags.set( MODELCONDITION_SPECIAL_DAMAGED );
		setFlags.set( MODELCONDITION_DAMAGED );
		structure->clearAndSetModelConditionFlags( clearFlags, setFlags );
		conditionIndex = GARRISON_POINT_DAMAGED;

		count = structure->getMultiLogicalBonePosition("FIREPOINT", MAX_GARRISON_POINTS, m_garrisonPoint[ conditionIndex ], NULL);

		if ( count > 0) gBonesFound = TRUE;

		// really damaged garrisoned
		clearFlags.clear();
		setFlags.clear();
		clearFlags.set( MODELCONDITION_RUBBLE );
		clearFlags.set( MODELCONDITION_SPECIAL_DAMAGED );
		clearFlags.set( MODELCONDITION_DAMAGED );
		setFlags.set( MODELCONDITION_REALLY_DAMAGED );
		structure->clearAndSetModelConditionFlags( clearFlags, setFlags );
		conditionIndex = GARRISON_POINT_REALLY_DAMAGED;

		count = structure->getMultiLogicalBonePosition("FIREPOINT", MAX_GARRISON_POINTS, m_garrisonPoint[ conditionIndex ], NULL);

		if ( count > 0) gBonesFound = TRUE;

		// restore the original condition flags
		draw->replaceModelConditionFlags( originalFlags );

	}  // end if, draw

	// garrison points are now initialized
	m_garrisonPointsInitialized = TRUE;



	if (gBonesFound && modData->m_mobileGarrison && (getObject()->isMobile() == TRUE) )
	{
		DEBUG_ASSERTCRASH( getObject()->isMobile() == FALSE,
		 ("GarrisonContain::update - You have specified this garrisonContain as mobile,\n yet you want garrison point placement bones... \n what are you thinking?") );
	}



}  // end loadGarrisonPoints

// ------------------------------------------------------------------------------------------------
/** Validate any exit rally point that has been chosen (if any).  If it's not valid,
	* try to find a new one */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::validateRallyPoint( void )
{

	// if we have a rally point already picked, make sure it's valid
	if( m_rallyValid == TRUE )
	{
		Coord3D result;
		FindPositionOptions options;

		// ask for a valid position exactly at the rally point
		options.flags = FPF_IGNORE_ALLY_OR_NEUTRAL_UNITS;
		options.minRadius = 0.0f;
		options.maxRadius = 0.0f;
		options.ignoreObject = getObject();
		options.relationshipObject = getObject();
		if( ThePartitionManager->findPositionAround( &m_exitRallyPoint, &options, &result ) == FALSE )
			m_rallyValid = FALSE;

	}  // end if

	// if no rally point is present, try to find one
	if( m_rallyValid == FALSE )
	{
		FindPositionOptions options;

		// pick a location for everybody to rally at
		options.flags = FPF_IGNORE_ALLY_OR_NEUTRAL_UNITS;
		options.minRadius = getObject()->getGeometryInfo().getBoundingCircleRadius();
		options.maxRadius = options.minRadius * 1.8f;  // arbitrary max distance away, change as needed
		options.ignoreObject = getObject();
		options.relationshipObject = getObject();
		m_rallyValid = ThePartitionManager->findPositionAround( getObject()->getPosition(),
																													  &options,
																													  &m_exitRallyPoint );
	}  // end if

}  // end validateRallyPoint

// ------------------------------------------------------------------------------------------------
/** Remove all contents of this container.  We will try to do so with intelligent garrison
	* logic, but if all else fails no matter, we need to get all things out after this
	* call is complete */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::removeAllContained( Bool exposeStealthUnits )
{

	//
	// we will call this when we are destroying the object (either normally or through a game
	// reset/exit etc) *and* also when we have received an "evacuate" command from the player.
	// We will attempt to find a spot for all the contents to scatter to that is around the
	// structure, but if such a spot is not found, we'll just scatter in a random direction
	// from the building.
	//

	// only even bother doing this if we have contents inside us just cause it's a waste
	if( getContainCount() > 0 )
	{

		// validate the current rally point is still a good one, or pick a new one
		validateRallyPoint();

	}  // end if

	// call the base class to extend functionality and do the actual removal
	OpenContain::removeAllContained( exposeStealthUnits );

	recalcApparentControllingPlayer();

}  // end removeAllContained

// ------------------------------------------------------------------------------------------------
/** 'exitObj' is one of the things we contain, it needs to 'exit' us */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::exitObjectViaDoor( Object *exitObj, ExitDoorType exitDoor )
{
	DEBUG_ASSERTCRASH(exitDoor == DOOR_1, ("multiple exit doors not supported here"));

	// We don't use the ExitPath system of the general OpenContain, we just send people out.  The
	// direction of outing has been picked by Design to be the Screen Down at the default camera angle.
	removeFromContain( exitObj );

	Coord3D startPosition;
	Coord3D endPosition;

	Real exitAngle = getObject()->getOrientation();
	// Garrison doesn't have reserveDoor or exitDelay, so if we do nothing, everyone will appear on top
	// of each other and get stuck inside each others' extent (except for the first guy).  So we'll
	// scatter the start point around a little to make it better.
	startPosition = *getObject()->getPosition();
	// In the case of cliff bunkers, the units start in a cliff.  So we want to adjust.
	AIUpdateInterface  *ai = exitObj->getAI();
	if (ai) {
		Locomotor *loco = ai->getCurLocomotor();
		if (loco && !TheAI->pathfinder()->validMovementTerrain( LAYER_GROUND, loco, &startPosition)) {
			// try front & back.
			Real offset = getObject()->getGeometryInfo().getMajorRadius();
			startPosition.x -= offset*Cos(exitAngle);
			startPosition.y -= offset*Sin(exitAngle);
			if (!TheAI->pathfinder()->validMovementTerrain(LAYER_GROUND, loco, &startPosition)) {
				startPosition.x += 2*offset*Cos(exitAngle);
				startPosition.y += 2*offset*Sin(exitAngle);
				if (!TheAI->pathfinder()->validMovementTerrain(LAYER_GROUND, loco, &startPosition)) {
					startPosition = *getObject()->getPosition();
				}
			}
		}
	}

	exitObj->setPosition( &startPosition );
	exitObj->setOrientation( exitAngle );
	///< @todo This really should be automatically wrapped up in an activation sequence	for objects in general
	// tell the AI about it
	TheAI->pathfinder()->addObjectToPathfindMap( exitObj );
	endPosition = startPosition;
	if( ai )
	{
		TheAI->pathfinder()->adjustToPossibleDestination(exitObj, ai->getLocomotorSet(), &endPosition);
		std::vector<Coord3D> exitPath;
		exitPath.push_back(endPosition);

		ai->aiFollowPath( &exitPath, getObject(), CMD_FROM_AI );
		TheAI->pathfinder()->updateGoal(exitObj, &endPosition, TheTerrainLogic->getLayerForDestination(&endPosition));
	}

	recalcApparentControllingPlayer();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GarrisonContain::onContaining( Object *obj )
{

	// extend base class
	OpenContain::onContaining( obj );

	// get the structure object
	Object *structure = getObject();

	// objects inside a building are held
	obj->setDisabled( DISABLED_HELD );

	// the building can now attack, since it has soldiers inside of it
	structure->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_CAN_ATTACK ) );

	// give the object a garrisoned version of its weapon
	obj->setWeaponBonusCondition( WEAPONBONUSCONDITION_GARRISONED );

	// put the object in the center of the building
	obj->setPosition( structure->getPosition() );

	//
	// the team of the building is now the same as those that have garrisoned it, be sure
	// to save our original team tho so that we can revert back to it when all the
	// occupants are gone
	//
	recalcApparentControllingPlayer();

}  // end onContaining

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GarrisonContain::onRemoving( Object *obj )
{
	OpenContain::onRemoving(obj);

	// first remove the object from any garrison fire point if it's at one
	removeObjectFromGarrisonPoint( obj );

	// give the object back a regular weapon
	obj->clearWeaponBonusCondition( WEAPONBONUSCONDITION_GARRISONED );

	// object is no longer held inside a garrisoned building
	obj->clearDisabled( DISABLED_HELD );

	// if we have nothing left inside of us then we are once again back to our original team
	if( getContainCount() == 0 )
	{

		// put us back on our original team
		// (hokey exception: if our team is null, don't bother -- this
		// usually means we are being called during game-teardown and
		// the teams are no longer valid...)
		if (getObject()->getTeam() != NULL)
		{
			getObject()->setTeam( m_originalTeam );
			m_originalTeam = NULL;
		}

		// we also lose our transient attack ability
		getObject()->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_CAN_ATTACK ) );
		m_hideGarrisonedStateFromNonallies = false;

		// change the state back from garrisoned
		Drawable *draw = getObject()->getDrawable();
		if( draw )
		{
			draw->clearModelConditionState( MODELCONDITION_GARRISONED );
		}

	}  // end if
	else if( getStealthUnitsContained() != getContainCount() )
	{
		m_hideGarrisonedStateFromNonallies = false;
	}

	// disable occlusion while the unit walks out of the building
	///@todo: we should probably not draw the unit to begin with and just have them pop out.
	obj->setSafeOcclusionFrame(TheGameLogic->getFrame()+obj->getTemplate()->getOcclusionDelay());

	recalcApparentControllingPlayer();
}  // end onRemoving

// ------------------------------------------------------------------------------------------------
/** A GarrisonContain always lets people shoot out */
// ------------------------------------------------------------------------------------------------
Bool GarrisonContain::isPassengerAllowedToFire() const
{

	return TRUE;

}  // end isPassengerAllowedToFire

// ------------------------------------------------------------------------------------------------
/** A Mobile garrison keeps its occupants with it when it moves */
//-------------------------------------------------------------------------------------------------
void GarrisonContain::moveObjectsWithMe( void )
{
	const GarrisonContainModuleData *modData = getGarrisonContainModuleData();

	if (!modData->m_mobileGarrison)
		return;

	const ContainedItemsList& containList = getContainList();
	for( ContainedItemsList::const_iterator it = containList.begin(); it != containList.end(); ++it )
	{
		Object *obj;
		// get the object
		obj = *it;
		obj->setPosition(getObject()->getPosition());
	}
}

//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void GarrisonContain::onBodyDamageStateChange( const DamageInfo* , BodyDamageType , BodyDamageType newState )
{
	// This is an event triggered on edge, so we just need to look at the newState. We know a change happened.
	// And we don't need to check Rubble, since death exiting is already handled. This is Garrison specific.
	if( newState == BODY_REALLYDAMAGED && !getObject()->isKindOf( KINDOF_GARRISONABLE_UNTIL_DESTROYED ) )
	{
		if( getContainCount() > 0 )
			orderAllPassengersToExit(CMD_FROM_AI);
	}
}

//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void GarrisonContain::onObjectCreated()
{
	GarrisonContainModuleData* self = (GarrisonContainModuleData*)getGarrisonContainModuleData();

	Int count = self->m_initialRoster.count;
	const ThingTemplate* rosterTemplate = TheThingFactory->findTemplate( self->m_initialRoster.templateName );
	Object* object = getObject();

	for( int i = 0; i < count; i++ )
	{
		//We are creating a garrison that comes with an initial roster, so add it now!
		Object* payload = TheThingFactory->newObject( rosterTemplate, object->getControllingPlayer()->getDefaultTeam() );
		if( object->getContain() && object->getContain()->isValidContainerFor( payload, true ) )
		{
			object->getContain()->addToContain( payload );
		}
		else
		{
			DEBUG_CRASH( ( "DeliverPayload: PutInContainer %s is full, or not valid for the payload %s!",
				object->getName().str(), self->m_initialRoster.templateName.str() ) );
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::crc( Xfer *xfer )
{

	// extend base class
	OpenContain::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::xfer( Xfer *xfer )
{
	Int i;

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	OpenContain::xfer( xfer );

	// original team
	TeamID teamID = m_originalTeam ? m_originalTeam->getID() : TEAM_ID_INVALID;
	xfer->xferUser( &teamID, sizeof( TeamID ) );
	if( xfer->getXferMode() == XFER_LOAD )
	{

		if( teamID != TEAM_ID_INVALID )
		{

			m_originalTeam = TheTeamFactory->findTeamByID( teamID );
			if( m_originalTeam == NULL )
			{

				DEBUG_CRASH(( "GarrisonContain::xfer - Unable to find original team by id" ));
				throw SC_INVALID_DATA;

			}  // end if

		}  // end if
		else
			m_originalTeam = NULL;

	}  // end if

	xfer->xferBool( &m_hideGarrisonedStateFromNonallies );

	// garrison point data
	UnsignedShort pointDataCount = MAX_GARRISON_POINTS;
	xfer->xferUnsignedShort( &pointDataCount );
	for( i = 0; i < pointDataCount; ++i )
	{

		if( xfer->getXferMode() == XFER_SAVE )
		{

			// object at this point
			Object *obj = m_garrisonPointData[ i ].object;
			ObjectID objectID = obj ? obj->getID() : INVALID_ID;
			xfer->xferObjectID( &objectID );

			// target
			xfer->xferObjectID( &m_garrisonPointData[ i ].targetID );

			// placement frame
			xfer->xferUnsignedInt( &m_garrisonPointData[ i ].placeFrame );

			// last effect frame
			xfer->xferUnsignedInt( &m_garrisonPointData[ i ].lastEffectFrame );

			// effect drawable id
			Drawable *draw = m_garrisonPointData[ i ].effect;
			DrawableID drawableID = draw ? draw->getID() : INVALID_DRAWABLE_ID;
			xfer->xferDrawableID( &drawableID );

		}  // end if, save
		else
		{

			// objectID
			ObjectID objectID;
			xfer->xferObjectID( &objectID );

			// target
			ObjectID targetID;
			xfer->xferObjectID( &targetID );

			// placement frame
			UnsignedInt placeFrame;
			xfer->xferUnsignedInt( &placeFrame );

			// last effect frame
			UnsignedInt lastEffectFrame;
			xfer->xferUnsignedInt( &lastEffectFrame );

			// effect drawable id
			DrawableID drawableID;
			xfer->xferDrawableID( &drawableID );

			// store
			if( i < MAX_GARRISON_POINTS )
			{

				m_garrisonPointData[ i ].objectID = objectID;
				m_garrisonPointData[ i ].targetID = targetID;
				m_garrisonPointData[ i ].placeFrame = placeFrame;
				m_garrisonPointData[ i ].lastEffectFrame = lastEffectFrame;
				m_garrisonPointData[ i ].effectID = drawableID;

			}  // end if

		}  // end else, load

	}  // end for i

	// garrison points in use
	xfer->xferInt( &m_garrisonPointsInUse );

	// garrison points
	xfer->xferUser( m_garrisonPoint, sizeof( Coord3D ) * MAX_GARRISON_POINT_CONDITIONS * MAX_GARRISON_POINTS );

	// garrison points initialized
	xfer->xferBool( &m_garrisonPointsInitialized );

	// rally valid
	xfer->xferBool( &m_rallyValid );

	// exit rally point
	xfer->xferCoord3D( &m_exitRallyPoint );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void GarrisonContain::loadPostProcess( void )
{

	// extend base class
	OpenContain::loadPostProcess();

	// connect up pointers needed
	for( Int i = 0; i < MAX_GARRISON_POINTS; i++ )
	{

		// object pointer
		if( m_garrisonPointData[ i ].objectID != INVALID_ID )
		{

			m_garrisonPointData[ i ].object = TheGameLogic->findObjectByID( m_garrisonPointData[ i ].objectID );
			if( m_garrisonPointData[ i ].object == NULL )
			{

				DEBUG_CRASH(( "GarrisonContain::loadPostProcess - Unable to find object for point data" ));
				throw SC_INVALID_DATA;

			}  // end if

		}  // end if
		else
			m_garrisonPointData[ i ].object = NULL;

		// drawable effect pointer
		if( m_garrisonPointData[ i ].effectID != INVALID_ID )
		{

			m_garrisonPointData[ i ].effect = TheGameClient->findDrawableByID( m_garrisonPointData[ i ].effectID );
			if( m_garrisonPointData[ i ].effect == NULL )
			{

				DEBUG_CRASH(( "GarrisonContain::loadPostProcess - Unable to find effect for point data" ));
				throw SC_INVALID_DATA;

			}  // end if

		}  // end if
		else
			m_garrisonPointData[ i ].effect = NULL;

	}  // end for i

}  // end loadPostProcess















