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

// FILE: POWTruckAIUpdate.cpp /////////////////////////////////////////////////////////////////////
// Author: Colin Day
// Desc:   AI for the POW Truck
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/ActionManager.h"
#include "Common/GlobalData.h"
#include "Common/Money.h"
#include "Common/Player.h"
#include "Common/ThingTemplate.h"
#include "GameClient/InGameUI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/OpenContain.h"
#include "GameLogic/Module/POWTruckAIUpdate.h"

#ifdef ALLOW_SURRENDER

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC /////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
POWTruckAIUpdateModuleData::POWTruckAIUpdateModuleData( void )
{

	m_boredTimeInFrames = 0;
	m_hangAroundPrisonDistance = 0;

}  // end POWTruckAIUpdateModuleData

// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdateModuleData::buildFieldParse( MultiIniFieldParse &p )
{
  AIUpdateModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] = 
	{
		{ "BoredTime",					INI::parseDurationUnsignedInt,	NULL, offsetof( POWTruckAIUpdateModuleData, m_boredTimeInFrames ) },
		{ "AtPrisonDistance",		INI::parseReal,		NULL,		offsetof( POWTruckAIUpdateModuleData, m_hangAroundPrisonDistance ) },
		{ 0, 0, 0, 0 }
	};

  p.add( dataFieldParse );

}  // end buildFieldParse

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC /////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
POWTruckAIUpdate::POWTruckAIUpdate( Thing *thing, const ModuleData *moduleData )
								: AIUpdateInterface( thing, moduleData )              
{

	m_aiMode = AUTOMATIC;
	m_currentTask = POW_TRUCK_TASK_WAITING;
	m_targetID = INVALID_ID;
	m_prisonID = INVALID_ID;
	m_enteredWaitingFrame = 0;
	m_lastFindFrame = 0;

}  // end POWTruckAIUpdate

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
POWTruckAIUpdate::~POWTruckAIUpdate( void )
{

}  // end ~POWTruckAIUpdate

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::onDelete( void )
{

}  // end onDelete

// ------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::aiDoCommand( const AICommandParms *parms )
{

	if (!isAllowedToRespondToAiCommands(parms))
		return;

	// if the command is from a player, stop whatever we might be doing and accept the new command
	if( parms->m_cmdSource == CMD_FROM_PLAYER )
	{

		// stop
		aiIdle( CMD_FROM_AI );

		// set our task to waiting
		setTask( POW_TRUCK_TASK_WAITING );

	}  // end if

	switch( parms->m_cmd )
	{

		// --------------------------------------------------------------------------------------------
		case AICMD_PICK_UP_PRISONER:
		{

			privatePickUpPrisoner( parms->m_obj, parms->m_cmdSource );
			break;

		}  // end pick up prisoner

		// --------------------------------------------------------------------------------------------
		case AICMD_RETURN_PRISONERS:
		{

			privateReturnPrisoners( parms->m_obj, parms->m_cmdSource );
			break;

		}  // end return prisoners

		// --------------------------------------------------------------------------------------------
		default:
		{

			// call the default do command
			AIUpdateInterface::aiDoCommand( parms );
			break;

		}  // end default

	}  // end switch( parms->m_cmd )

}  // end aiDoCommand

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime POWTruckAIUpdate::update( void )
{

	// we are ultra accurate
	if( getCurLocomotor() )
		getCurLocomotor()->setUltraAccurate( TRUE );

	// extend the regular ai update module
	UpdateSleepTime result;
	result = AIUpdateInterface::update();

	// being able to path through units makes the POW truck more effective
	// but we're getting rid of this functionality (CBD 10-10-2002)
// 	setCanPathThroughUnits(true);

	// do our own logic
	switch( m_currentTask )
	{

		case POW_TRUCK_TASK_WAITING:
			updateWaiting();
			break;
		case POW_TRUCK_TASK_FIND_TARGET:
			updateFindTarget();
			break;
		case POW_TRUCK_TASK_COLLECTING_TARGET:
			updateCollectingTarget();
			break;
		case POW_TRUCK_TASK_RETURNING_PRISONERS:
			updateReturnPrisoners();
			break;
		default:
			DEBUG_CRASH(( "POWTruckAIUpdate::update - Unknown current task '%d'", m_currentTask ));
			break;

	}  // end switch, current task

	return UPDATE_SLEEP_NONE;

}  // end update

///////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED //////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
/** Set our current task */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::setTask( POWTruckTask task, Object *taskObject )
{
	POWTruckTask oldTask = m_currentTask;

	// sanity, POW_TRUCK_TASK_COLLECTING_TARGET and POW_TRUCK_TASK_RETURNING_PRISONERS require taskObject parameters
	if( (task == POW_TRUCK_TASK_COLLECTING_TARGET || task == POW_TRUCK_TASK_RETURNING_PRISONERS) && 
			taskObject == NULL )
	{

		DEBUG_CRASH(( "POWTruckAIUpdate::setTask - Illegal arguments" ));
		setTask( POW_TRUCK_TASK_WAITING );
		return;

	}  // end if

	// when leaving the collecting target state, we need to do some bookeeping
	if( oldTask == POW_TRUCK_TASK_COLLECTING_TARGET )
	{

		// target cleanup
		m_targetID = INVALID_ID;

	}  // end if

	// to be clean, when leaving the POW_TRUCK_TASK_RETURNING_PRISONERS state, clear out the prison ID we were watching
	if( oldTask == POW_TRUCK_TASK_RETURNING_PRISONERS )
		m_prisonID = INVALID_ID;

	// we store IDs for some tasks
	if( task == POW_TRUCK_TASK_COLLECTING_TARGET )
	{

		// save ID of our target
		m_targetID = taskObject->getID();

		// mark this target as slated for pickup
		DEBUG_ASSERTCRASH( taskObject->getAIUpdateInterface(), ("POWTruckAIUpdate::setTask - '%s' has no ai module",
													 taskObject->getTemplate()->getName().str()) );

	}  // end if
	else if( task == POW_TRUCK_TASK_RETURNING_PRISONERS )
	{

		// save ID of the prison we're going to
		m_prisonID = taskObject->getID();

	}  // end else
	else if( task == POW_TRUCK_TASK_WAITING )
	{

		// mark the frame and always go idle
		m_enteredWaitingFrame = TheGameLogic->getFrame();

	}  // end else if

	// store the new current task
	m_currentTask = task;

}  // end task

// ------------------------------------------------------------------------------------------------
/** Set this unit into automatic or manual AI mode, when automatic the AI will look for 
	* prisoners, collect them, and bring them back automatically.  When in manual, the 
	* player must direct the actions of the unit explicitly */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::setAIMode( POWTruckAIMode mode )
{

	// save the mode
	m_aiMode = mode;

}  // end setAIModel

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::privatePickUpPrisoner( Object *prisoner, CommandSourceType cmdSource )
{

	// if 'prisoner' is not a valid target for picking up don't bother going forward
	if( validateTarget( prisoner ) == FALSE )
		return;

	//
	// we will now try to pick up this target ... if the command was from a player then
	// we are no longer in automatic mode and will not return to automatic mode until
	// the player issues the command for us to return back to prison
	//
	// CBD: Disabling this whole manual thing for now
	//
	if( FALSE && cmdSource == CMD_FROM_PLAYER )
		setAIMode( MANUAL );
	else
		setAIMode( AUTOMATIC );

	// set our task as picking up our target and save the target ID
	setTask( POW_TRUCK_TASK_COLLECTING_TARGET, prisoner );

	// move to the target
	ignoreObstacle( prisoner );
	if( getCurLocomotor() )
		getCurLocomotor()->setUltraAccurate( TRUE );
	aiMoveToObject( prisoner, CMD_FROM_AI );

}  // end privatePickUpPrisoner

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::privateReturnPrisoners( Object *prison, CommandSourceType cmdSource )
{

	// when receiving this command from the player, we will always enter AUTOMATIC ai mode
	if( cmdSource == CMD_FROM_PLAYER )
		setAIMode( AUTOMATIC );

	// if no prison is provided, find one if possible
	if( prison == NULL )
		prison = findBestPrison();

	// still no prison, nothing to do
	if( prison == NULL )
		return;

	// set us into the return prisoners "state"
	setTask( POW_TRUCK_TASK_RETURNING_PRISONERS, prison );

	// dock with the prison
	if( getCurLocomotor() )
		getCurLocomotor()->setUltraAccurate( TRUE );
	aiDock( prison, cmdSource );

}  // end privateReturnPrisoners

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::updateWaiting( void )
{

	// if we're manual control ... do nothing
	if( m_aiMode == MANUAL )
		return;

	// get our info
	Object *us = getObject();
	AIUpdateInterface *ai = us->getAIUpdateInterface();
	DEBUG_ASSERTCRASH( ai, ("POWTruckAIUpdate::updateWaiting - '%s' has no ai",
													us->getTemplate()->getName().str()) );

	//
	// if we're not idle we don't consider ourselves waiting ... we could be moving from
	// a players command, or just about anything ... so just keep updating the m_enteredWaitingFrame
	// to the current frame until we actually become idle and start to wait
	//
	if( ai->isIdle() == FALSE )
		m_enteredWaitingFrame = TheGameLogic->getFrame();

	// get module data
	const POWTruckAIUpdateModuleData *modData = getPOWTruckAIUpdateModuleData();

	// if enough time has passed we're bored of waiting, try to find another target
	if( TheGameLogic->getFrame() - m_enteredWaitingFrame > modData->m_boredTimeInFrames )
		setTask( POW_TRUCK_TASK_FIND_TARGET );

}  // updateWaiting

static const UnsignedInt FIND_DELAY = LOGICFRAMES_PER_SECOND * 1;
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::updateFindTarget( void )
{

	// we never find targets when in manual ai mode
	DEBUG_ASSERTCRASH( m_aiMode != MANUAL, ("POWTruckAIUpdate::updateFindTarget - We shouldn't be here with a manual ai mode") );
	if( m_aiMode == MANUAL )
		return;

	// don't do this too often
	if( TheGameLogic->getFrame() - m_lastFindFrame < FIND_DELAY )
		return;

	// we've now done a find
	m_lastFindFrame = TheGameLogic->getFrame();

	// get our info
	Object *us = getObject();
	AIUpdateInterface *ai = us->getAIUpdateInterface();
	DEBUG_ASSERTCRASH( ai, ("POWTruckAIUpdate::updateFindTarget - '%s' has no ai",
													us->getTemplate()->getName().str()) );

	// if we're full we should return to prison
	ContainModuleInterface *contain = us->getContain();
	if( contain && contain->getContainCount() == contain->getContainMax() )
	{

		doReturnPrisoners();
		return;

	}  // end if

	// find closest target not already targeted for pickup by another POW truck
	Object *target = findBestTarget();

	// if target exists, go get it
	if( target )
	{
		
		// send the pickup command
		ai->aiPickUpPrisoner( target, CMD_FROM_AI );

	}  // end if
	else
	{
	
		// no target exists, if we have prisoners return them
		if( contain->getContainCount() != 0 )
			doReturnPrisoners();
		else
			doReturnToPrison( NULL );

	}  // end else
			
}  // end updateFindTarget

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::updateCollectingTarget( void )
{

	// validate our target is still OK to collect
	Object *target = TheGameLogic->findObjectByID( m_targetID );
	if( validateTarget( target ) == FALSE )
	{

		// find more prisoners or wait for player intervention
		if( m_aiMode == AUTOMATIC )
			setTask( POW_TRUCK_TASK_FIND_TARGET );
		else
			setTask( POW_TRUCK_TASK_WAITING );

		return;

	}  // end if

	// we should never become idle while collecting a target, if we do, find a target
	Object *us = getObject();
	AIUpdateInterface *ai = us->getAIUpdateInterface();
	if( ai->isIdle() )
	{

		if( m_aiMode == AUTOMATIC )
			setTask( POW_TRUCK_TASK_FIND_TARGET );
		else
			setTask( POW_TRUCK_TASK_WAITING );

	}  // end if

/*
	//
	// when we're close to the target, we tell the target to start walking toward
	// us ... if they're not idle we don't do this as they are already moving
	//
	Object *us = getObject();
	AIUpdateInterface *targetAI = target->getAIUpdateInterface();
	if( targetAI->isIdle() )
	{

		// are we close enought to tell them to start moving to us
		Real distSq = pow( us->getGeometryInfo().getBoundingSphereRadius() * 2.0f, 2 );
		if( ThePartitionManager->getDistanceSquared( us, target, FROM_CENTER_2D ) <= distSq )
		{

			// tell them to start moving to us
			targetAI->aiMoveToObject( us, CMD_FROM_AI );

		}  // end if

	}  // end if
*/

}  // end updateCollectingTarget

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
struct PrisonerReturnData
{
	Object *source;			///< objects will be removed from source and put into dest
	Object *dest;				///< objects will be removed from source and put into dest
};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static void putContainedInPrison( Object *obj, void *userData )
{
	PrisonerReturnData *returnData = (PrisonerReturnData *)userData;

	// sanity
	DEBUG_ASSERTCRASH( returnData != NULL && returnData->source != NULL && returnData->dest != NULL,
										 ("putContainedInPrison: Invalid arguments") );

	// take 'obj' out of the source
	ContainModuleInterface *sourceContain = returnData->source->getContain();
	if( sourceContain )
	{
		sourceContain->removeFromContain( obj );
	}

	// put 'obj' into dest
	ContainModuleInterface *destContain = returnData->dest->getContain();
	if( destContain )
	{
		destContain->addToContain( obj );
	}

}  // end putContainedInPrison

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::updateReturnPrisoners( void )
{
	Object *us = getObject();
	AIUpdateInterface *ai = us->getAIUpdateInterface();
	DEBUG_ASSERTCRASH( ai, ("POWTruckAIUpdate::updateReturnPrisoners - '%s' has no AI",
												 us->getTemplate()->getName().str()) );

	// get the prison we're returning to
	Object *prison = TheGameLogic->findObjectByID( m_prisonID );
	
	// prison has gone away, do this all over again
	if( prison == NULL )
	{

		doReturnPrisoners();
		return;

	}  // end if

	// we should not go idle, we should collide with the prison to return the prisoners
	if( ai->isIdle() )
		doReturnPrisoners();

}  // end updateReturnPrisoners

// ------------------------------------------------------------------------------------------------
/** Is the object 'target' a valid target for collection */
// ------------------------------------------------------------------------------------------------
Bool POWTruckAIUpdate::validateTarget( const Object *target )
{

	return TheActionManager->canPickUpPrisoner( getObject(), target, getLastCommandSource() );

}  // end validateTarget

// ------------------------------------------------------------------------------------------------
/** Initiate a return prisoners contained in us to prison */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::doReturnPrisoners( void )
{
	
	// find the closest prison
	Object *prison = findBestPrison();

	// if no prison is available, nothing to do
	if( prison == NULL )
	{

		setTask( POW_TRUCK_TASK_WAITING );
		return;

	}  // end if

	// start the prisoner return process
	Object *us = getObject();
	AIUpdateInterface *ai = us->getAIUpdateInterface();
	DEBUG_ASSERTCRASH( ai, ("POWTruckAIUpdate::doReturnPrisoners - '%s' has no AI",
												 us->getTemplate()->getName().str()) );
	ai->aiReturnPrisoners( prison, CMD_FROM_AI );

}  // end doReturnPrisoners

// ------------------------------------------------------------------------------------------------
/** Initate a return of our empty truck back near the closest prison */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::doReturnToPrison( Object *prison )
{

	//
	// the great thing about this logic is that there isn't a state to "return to prison", we
	// just go into waiting, find a prison if we can, and tell the AI to move there
	//
	setTask( POW_TRUCK_TASK_WAITING );

	// find the closest prison if one was not provided
	if( prison == NULL )
		prison = findBestPrison();

	// if no prison found forget it
	if( prison == NULL )
		return;

	// get our info
	Object *us = getObject();
	const POWTruckAIUpdateModuleData *modData = getPOWTruckAIUpdateModuleData();

	// if we're close enough to it then just stay here
	Real distSq = ThePartitionManager->getDistanceSquared( us, prison, FROM_CENTER_2D );
	if( distSq <= modData->m_hangAroundPrisonDistance * modData->m_hangAroundPrisonDistance )
		return;

	// dock us with the prison
	aiDock( prison, CMD_FROM_AI );

}  // end doReturnToPrison

// ------------------------------------------------------------------------------------------------
/** Find the best prison for us to use given our current position */
// ------------------------------------------------------------------------------------------------
Object *POWTruckAIUpdate::findBestPrison( void )
{

	ObjectID prisonID = getObject()->getProducerID();
	if( prisonID == INVALID_ID )
		return NULL;

	// find prison object
	Object *prison = TheGameLogic->findObjectByID( prisonID );

	return prison;

}  // end findBestPrison

// ------------------------------------------------------------------------------------------------
/** Find the best prisoner for us to go pick up given our current situation */
// ------------------------------------------------------------------------------------------------
Object *POWTruckAIUpdate::findBestTarget( void )
{
	const Object *us = getObject();
	Player *player = us->getControllingPlayer();

	// sanity
	if( player == NULL )
		return NULL;

	// get our info
	const AIUpdateInterface *ai = us->getAIUpdateInterface();
	DEBUG_ASSERTCRASH( ai, ("POWTruckAIUpdate::findBestTarget- '%s' has no AI",
												 us->getTemplate()->getName().str()) );

	// scan all objects, there is no range
	Object *other;
	Real closestTargetDistSq = HUGE_DIST;
	Object *closestTarget = NULL;
	for( other = TheGameLogic->getFirstObject(); other; other = other->getNextObject() )
	{

		//
		// run this target through our validation scheme and don't pick it in the first
		// place if it's not a valid target for us
		//
		if( validateTarget( other ) == FALSE )
			continue;

		// ignore targets that we cannot pathfind to as we will never be able to pick them up
		if( isQuickPathAvailable( other->getPosition() ) == FALSE )
			continue;

		// is this target closer than the one we've found so far
		Real distSq = ThePartitionManager->getDistanceSquared( us, other, FROM_CENTER_2D ); 
		if( closestTarget == NULL || distSq < closestTargetDistSq )
		{

			// we must be able to pathfind to this target 
			if( TheAI->pathfinder()->quickDoesPathExist( ai->getLocomotorSet(),
																							us->getPosition(), 
																							other->getPosition() ) == TRUE )
			{

				// this is our new closest target
				closestTarget = other;
				closestTargetDistSq = distSq;

			}  // end if

		}  // end if

	}  // end for other

	return closestTarget;

}  // end findBestTarget

// ------------------------------------------------------------------------------------------------
/** We are chosing to pass a structure through the iterate function to unload the
	* prisoners because it's more flexible in that, perhaps someday in the future, we
	* could fail to add an object to the prison (maybe it's full or something), and in
	* that case it's better to manually do actions each time a successful
	* add happens rather than figure it out from before counts and after counts of the truck */
// ------------------------------------------------------------------------------------------------
struct PrisonUnloadData
{

	Object *sourceTruck;
	Object *destPrison;
	UnsignedInt bounty;

};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static void putPrisonersInPrison( Object *obj, void *userData )
{
	PrisonUnloadData *prisonUnloadData = (PrisonUnloadData *)userData;
	Object *prison = prisonUnloadData->destPrison;

	// sanity
	DEBUG_ASSERTCRASH( prison, ("putPrisonersInPrison: NULL user data") );
	DEBUG_ASSERTCRASH( obj->getContainedBy() != NULL,
										 ("putPrisonersInPrison: Prisoner '%s' is not contained by anything, it should be contained by a POW truck",
										 obj->getTemplate()->getName().str()) );

	// extra super sanity, just so that we don't crash ... this is in the assert above
	if( obj->getContainedBy() == NULL )
		return;

	// take 'obj' out of the truck
	ContainModuleInterface *sourceContain = obj->getContainedBy()->getContain();
	if( sourceContain )
	{
		sourceContain->removeFromContain( obj );
	}

	// put 'obj' into 'prison'
	ContainModuleInterface *destContain = prison->getContain();
	if( destContain )
	{
		destContain->addToContain( obj );
	}

	// add up bounty for this prisoner
	Player *prisonerOwningPlayer = obj->getControllingPlayer();
	prisonUnloadData->bounty += TheGlobalData->m_prisonBountyMultiplier * 
															obj->getTemplate()->calcCostToBuild( prisonerOwningPlayer );

}  // end putPrisonersInPrison

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::unloadPrisonersToPrison( Object *prison )
{
	Object *us = getObject();

	// sanity
	if( prison == NULL )
		return;

	// get contain modules
	ContainModuleInterface *truckContain = us->getContain();

	// sanity
	DEBUG_ASSERTCRASH( prison->getContain(), ("POWTruckAIUpdate::unloadPrisonersToPrison - '%s' has no contain",
																		prison->getTemplate()->getName().str()) );
	DEBUG_ASSERTCRASH( prison->getContain()->asOpenContain(), 
										 ("POWTruckAIUpdate::unloadPrisonersToPrison - '%s' has no OPEN contain",
										 prison->getTemplate()->getName().str()) );
	DEBUG_ASSERTCRASH( truckContain, ("POWTruckAIUpdate::unloadPrisonersToPrison - '%s' has no contain",
																	 us->getTemplate()->getName().str()) );
	DEBUG_ASSERTCRASH( truckContain->asOpenContain(), 
										 ("POWTruckAIUpdate::unloadPrisonersToPrison - '%s' has no OPEN contain",
										 us->getTemplate()->getName().str()) );

	// put the prisoners in the prison
	PrisonUnloadData prisonUnloadData;
	prisonUnloadData.sourceTruck = us;
	prisonUnloadData.destPrison = prison;
	prisonUnloadData.bounty = 0;
	truckContain->iterateContained( putPrisonersInPrison, &prisonUnloadData, FALSE );

	// if the destination prison collects a bounty, award that to the player
	if( prison->isKindOf( KINDOF_COLLECTS_PRISON_BOUNTY ) &&
			prisonUnloadData.bounty > 0 )
	{
		Player *player = us->getControllingPlayer();

		if( player )
		{
			Money *money = player->getMoney();

			if( money )
			{
				
				// deposit the money
				money->deposit( prisonUnloadData.bounty );
				player->getScoreKeeper()->addMoneyEarned( prisonUnloadData.bounty );

				// display text above the building
				Color moneyColor = TheGlobalData->m_prisonBountyTextColor;
				UnicodeString moneyString;
				Coord3D pos = *prison->getPosition();

				pos.z += prison->getGeometryInfo().getMaxHeightAbovePosition();
				moneyString.format( TheGameText->fetch( "GUI:AddCash" ), prisonUnloadData.bounty );
				TheInGameUI->addFloatingText( moneyString, &pos, moneyColor );

			}  // end if

		}  // end if

	}  // end if

	// set our state to waiting and become idle
	setTask( POW_TRUCK_TASK_WAITING );

}  // end unloadPrisonersToPrison

// ------------------------------------------------------------------------------------------------
/** We have arrived at a prisoner to load into us ... do it */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::loadPrisoner( Object *prisoner )
{
	Object *us = getObject();

	// sanity
	if( prisoner == NULL )
		return;

	// validate that we can load this prisoner
	if( validateTarget( prisoner ) == FALSE )
		return;

	// sanity check ... make sure we have room
	ContainModuleInterface *contain = us->getContain();
	if( contain && contain->getContainCount() == contain->getContainMax() )
	{

		// we're full ... return to prison
		doReturnPrisoners();
		return;

	}  // end if

	// add object to our containment
	contain->addToContain( prisoner );

	//
	// removed the surrendered status of these prisoners to false, that way if the truck
	// is killed, the prison blows up releasing all the prisoners, whatever, they will
	// behave as normal
	//
	AIUpdateInterface *prisonerAI = prisoner->getAIUpdateInterface();
	if( prisonerAI )
		prisonerAI->setSurrendered( NULL, FALSE );

	// done adding prisoner, for automatic AI find another target, for manual just wait
	if( m_aiMode == AUTOMATIC )
		setTask( POW_TRUCK_TASK_FIND_TARGET );
	else
		setTask( POW_TRUCK_TASK_WAITING );

}  // end loadPrisoner

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::crc( Xfer *xfer )
{
	// extend base class
	AIUpdateInterface::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );
 
 // extend base class
	AIUpdateInterface::xfer(xfer);

	xfer->xferUser(&m_aiMode, sizeof(m_aiMode));
	xfer->xferUser(&m_currentTask, sizeof(m_currentTask));
	xfer->xferObjectID(&m_targetID);
	xfer->xferObjectID(&m_prisonID);
	xfer->xferUnsignedInt(&m_enteredWaitingFrame);
	xfer->xferUnsignedInt(&m_lastFindFrame);

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void POWTruckAIUpdate::loadPostProcess( void )
{
 // extend base class
	AIUpdateInterface::loadPostProcess();
}  // end loadPostProcess

#endif
