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

// FILE: DozerAIUpdate.cpp ////////////////////////////////////////////////////////////////////////
// Author: Colin Day, February 2002
// Desc:   Dozer AI behavior
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/ActionManager.h"
#include "Common/Team.h"
#include "Common/StateMachine.h"
#include "Common/BuildAssistant.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/Player.h"
#include "Common/Money.h"
#include "Common/Radar.h"
#include "Common/RandomValue.h"
#include "Common/GameState.h"
#include "Common/GlobalData.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameText.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/BridgeBehavior.h"
#include "GameLogic/Module/BridgeTowerBehavior.h"
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/DozerAIUpdate.h"
#include "GameClient/InGameUI.h"


// FORWARD DECLARATIONS ///////////////////////////////////////////////////////////////////////////
class DozerPrimaryStateMachine;
class DozerActionStateMachine;

static const Real MIN_ACTION_TOLERANCE = 70.0f;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
/** Available Dozer actions */
//-------------------------------------------------------------------------------------------------
enum DozerActionType CPP_11(: Int)
{
	DOZER_ACTION_PICK_ACTION_POS,		///< pick a location "around" the target to do our action
	DOZER_ACTION_MOVE_TO_ACTION_POS,///< move to our action pos we've picked
	DOZER_ACTION_DO_ACTION,					///< do our action at the position, build/repair/etc ...
};

//-------------------------------------------------------------------------------------------------
/** Dozer picks a position around the target to do the action */
//-------------------------------------------------------------------------------------------------
class DozerActionPickActionPosState : public State
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DozerActionPickActionPosState, "DozerActionPickActionPosState")		

public:

	DozerActionPickActionPosState( StateMachine *machine, DozerTask task );
	virtual StateReturnType update( void );

protected:
	// snapshot interface
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();

protected:
	
	DozerTask m_task;							///< our task
	Int m_failedAttempts;					/**< counter for successive unsuccessfull attempts to pick 
																		 and move to an action position */

};
EMPTY_DTOR(DozerActionPickActionPosState)

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DozerActionPickActionPosState::DozerActionPickActionPosState( StateMachine *machine,
																															DozerTask task ) :
															 State( machine, "DozerActionPickActionPosState" )
{

	m_task = task;
	m_failedAttempts = 0;

}  // end DozerActionPickActionPosState

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerActionPickActionPosState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DozerActionPickActionPosState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferUser(&m_task, sizeof(m_task));
	xfer->xferInt(&m_failedAttempts);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerActionPickActionPosState::loadPostProcess( void )
{
}  // end loadPostProcess

//-------------------------------------------------------------------------------------------------
/** Pick a position around the target */
//-------------------------------------------------------------------------------------------------
StateReturnType DozerActionPickActionPosState::update( void )
{
	StateMachine *machine = getMachine();
	Object *dozer = machine->getOwner();

	// get dozer ai update interface
	AIUpdateInterface *ai = dozer->getAIUpdateInterface();
	if( !ai )
	{
		return STATE_FAILURE;
	}
	DozerAIInterface *dozerAI = ai->getDozerAIInterface();
	if( !dozerAI )
	{
		return STATE_FAILURE;
	}

	// get the object we're concerned with for our task
	Object *goalObject = TheGameLogic->findObjectByID( dozerAI->getTaskTarget( m_task ) );

	// if there is no goal, get out of this machine with a failure code (success is done in the action state )
	if( goalObject == NULL )	
	{

		// to be clean get rid of the goal object we set
		getMachine()->setGoalObject( NULL );

		// cancel our task
		dozerAI->cancelTask( m_task );

		// exit with failure
		return STATE_FAILURE;

	}  // end if

	// pick a location to move to
	Coord3D goalPos;
	const Coord3D *pos = dozerAI->getDockPoint( m_task, DOZER_DOCK_POINT_START );
	if( pos )
		goalPos = *pos;
	else
	{

		// pick a spot to use

		//
		// find the vector from goal object to us ... we will start our search on this angle so
		// that we "approach" a closer point rather than a point on a random side
		//
		Coord2D v;
		v.x = dozer->getPosition()->x - goalObject->getPosition()->x;
		v.y = dozer->getPosition()->y - goalObject->getPosition()->y;
	
		Real radius = goalObject->getGeometryInfo().getBoundingSphereRadius();
		FindPositionOptions fpOptions;
		fpOptions.minRadius = radius;
		fpOptions.maxRadius = radius;
		fpOptions.startAngle = v.toAngle();
		if( ThePartitionManager->findPositionAround( goalObject->getPosition(),
																								 &fpOptions,
																								 &goalPos ) == FALSE )
		{

			// return STATE_FAILURE; no, we don't ever want dozers to fail, particularly
			// if ai.
			goalPos = *goalObject->getPosition();

		}  // end if	

		//
		// we only ignore the goal object when we did the point selection in this more
		// "random" method cause we could pick a place inside the object
		//
		ai->ignoreObstacle( goalObject );

	}  // end if

	// set goal position and object
	machine->setGoalObject( goalObject );
	machine->setGoalPosition( &goalPos );
	ai->ignoreObstacle(goalObject);
	ai->aiMoveToPosition( &goalPos, CMD_FROM_AI );

	return STATE_SUCCESS;

}  // end update

//-------------------------------------------------------------------------------------------------
/** Dozer moves to the action position */
//-------------------------------------------------------------------------------------------------
class DozerActionMoveToActionPosState : public State
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DozerActionMoveToActionPosState, "DozerActionMoveToActionPosState")		

public:

	DozerActionMoveToActionPosState( StateMachine *machine, DozerTask task ) : State( machine, "DozerActionMoveToActionPosState" ) { m_task = task; }
	virtual StateReturnType update( void );

protected:
	// snapshot interface
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();

protected:

	DozerTask m_task;						///< our task

};
EMPTY_DTOR(DozerActionMoveToActionPosState)

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerActionMoveToActionPosState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DozerActionMoveToActionPosState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferUser(&m_task, sizeof(m_task));
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerActionMoveToActionPosState::loadPostProcess( void )
{
}  // end loadPostProcess

//-------------------------------------------------------------------------------------------------
/** We are supposed to be on route to our action position now, see when we get there or
	* detect that we have encountered a problem that's going to cause it to give up */
//-------------------------------------------------------------------------------------------------
StateReturnType DozerActionMoveToActionPosState::update( void )
{
	Object *goalObject = getMachineGoalObject();
	Object *dozer = getMachineOwner();

	// sanity
	if( goalObject == NULL || dozer == NULL )
		return STATE_FAILURE;

	AIUpdateInterface *ai = dozer->getAIUpdateInterface();
	ObjectID currentRepairer = goalObject->getSoleHealingBenefactor(); 
	if( m_task==DOZER_TASK_REPAIR && currentRepairer != INVALID_ID && currentRepairer != dozer->getID() )//oops I guess someone beat me to it!
	{
		if ( ai )	
		{
			DozerAIInterface *dozerAI = ai->getDozerAIInterface();
			if ( dozerAI )
				dozerAI->internalTaskComplete( m_task );
		}
		getMachine()->setGoalObject( NULL );
		return STATE_FAILURE;
	}

	// Double oops, someone else has taken over the build responsibility for this building.  We can't double up.
	// This can happen because a guy who is moving to build who is told to build will idle for a frame after registering.
	// Two workers, two started buildings.
	// Grab both workers, resume on one building
	// First will register, second will see first as active and say no
	// Grab both, click on other building
	// First will register, and idle for a frame, second will see first as not active and say yes
	// Next frame, first will start the build task, without reasking validity
	// Infinite number of workers can be told to build something with just two in progress buildings
	if( (m_task == DOZER_TASK_BUILD) && goalObject && (goalObject->getBuilderID() != dozer->getID()) )
	{
		// Geebus.  Returning failure is ignored, so you have to explicitly make the ai stop trying to restart the machine
		if ( ai )	
		{
			DozerAIInterface *dozerAI = ai->getDozerAIInterface();
			if ( dozerAI )
				dozerAI->internalTaskComplete( m_task );
		}
		getMachine()->setGoalObject( NULL );
		return STATE_FAILURE;
	}

	// if distance between us and our goal position is close enough
	const Coord3D *goalPos = getMachine()->getGoalPosition();
	Real distSqr = ThePartitionManager->getDistanceSquared( dozer, goalPos, FROM_BOUNDINGSPHERE_2D );
	const Real SLOP = 15.0f;
	Real allowableDistanceSqr = sqr(max( MIN_ACTION_TOLERANCE, dozer->getGeometryInfo().getBoundingSphereRadius() + SLOP ));


	if( distSqr <= allowableDistanceSqr )
	{
		if( m_task == DOZER_TASK_BUILD )
		{

			//
			// the object is now no longer awaiting construction, it is being constructed ... note
			// that we might possibly be here multiple times for a single object being built
			// but the setting of the same model condition doesn't have any adverse effects
			//
			goalObject->clearAndSetModelConditionFlags(
				MAKE_MODELCONDITION_MASK(MODELCONDITION_AWAITING_CONSTRUCTION), 
				MAKE_MODELCONDITION_MASK2(MODELCONDITION_PARTIALLY_CONSTRUCTED, MODELCONDITION_ACTIVELY_BEING_CONSTRUCTED));

		}  // end if

		return STATE_SUCCESS;

	}  // end if


	// if we're in the idle state fail our move
	// Failure transition is back to DOZER_ACTION_PICK_ACTION_POS, so 
	// it is ok to fail. jba.
	if( ai && ai->isIdle() )
		return STATE_FAILURE;

	return STATE_CONTINUE;

}  // end update

//-------------------------------------------------------------------------------------------------
/** Dozer does the "action" */
//-------------------------------------------------------------------------------------------------
class DozerActionDoActionState : public State
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DozerActionDoActionState, "DozerActionDoActionState")		

public:

	DozerActionDoActionState( StateMachine *machine, DozerTask task ) : State( machine, "DozerActionDoActionState" ) 
	{ 
		m_task = task; 
		//Added By Sadullah Nader
		// Initializations missing and needed
		m_enterFrame = 0;
	}
	virtual StateReturnType update( void );
	virtual StateReturnType onEnter( void );
	virtual void onExit( StateExitType status ) { }

protected:
	// snapshot interface
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();

protected:
	
	DozerTask m_task;						///< our task
	UnsignedInt m_enterFrame;		///< frame we entered this state on

};
EMPTY_DTOR(DozerActionDoActionState)

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerActionDoActionState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DozerActionDoActionState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferUser(&m_task, sizeof(m_task));
	xfer->xferUnsignedInt(&m_enterFrame);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerActionDoActionState::loadPostProcess( void )
{
}  // end loadPostProcess

//-------------------------------------------------------------------------------------------------
/** Entering the do action state */
//-------------------------------------------------------------------------------------------------
StateReturnType DozerActionDoActionState::onEnter( void )
{
	Object *dozer = getMachineOwner();
	DozerAIInterface *dozerAI = dozer->getAIUpdateInterface()->getDozerAIInterface();
	if( !dozerAI )
	{
		return STATE_FAILURE;
	}
	// DozerAIUpdate *dozerAI = static_cast<DozerAIUpdate *>(dozer->getAIUpdateInterface());
	
	// record the frame we came in on
	m_enterFrame = TheGameLogic->getFrame();

	// when building, we have additional movement that we will for docking
	if( m_task == DOZER_TASK_BUILD )
		dozerAI->setBuildSubTask( DOZER_SELECT_BUILD_DOCK_LOCATION );

	return STATE_CONTINUE;

}  // end onEnter

//-------------------------------------------------------------------------------------------------
/** Do the action */
//-------------------------------------------------------------------------------------------------
StateReturnType DozerActionDoActionState::update( void )
{
	Object *goalObject = getMachineGoalObject();
	Object *dozer = getMachineOwner();
	AIUpdateInterface *ai = dozer->getAIUpdateInterface();
	if( !ai )
	{
		return STATE_FAILURE;
	}

	DozerAIInterface *dozerAI = ai->getDozerAIInterface();
	if( !dozerAI )
	{
		return STATE_FAILURE;
	}
	// DozerAIUpdate *dozerAI = static_cast<DozerAIUpdate *>(dozer->getAIUpdateInterface());
//	const UnsignedInt ACTION_TIME = LOGICFRAMES_PER_SECOND * 4 ; // frames to spend here in this state doing the action

	// check for object gone
	if( goalObject == NULL )
		return STATE_FAILURE;

	if ( dozer->isDisabledByType( DISABLED_UNMANNED ) )// Yipes, I've been sniped!
		return STATE_FAILURE;

	// do the task
	Bool complete = FALSE;
	switch( m_task )
	{

		//---------------------------------------------------------------------------------------------
		case DOZER_TASK_BUILD:
		{
			//GS Moved this inside Build, since you are allowed to Repair things that are not your player (canRepairObject handles it)
			if (dozer->getControllingPlayer() != goalObject->getControllingPlayer())//Yipes, SOmehow I have changed sides in mid build!
				return STATE_FAILURE;

			// if we need to select the dock location and move there do so
			if( dozerAI->getBuildSubTask() == DOZER_SELECT_BUILD_DOCK_LOCATION )
			{
				const Coord3D *dockLocation = dozerAI->getDockPoint( m_task, DOZER_DOCK_POINT_ACTION );
				if( dockLocation )
					ai->aiMoveToPosition( dockLocation, CMD_FROM_AI );

				// we're now moving to the dock location
				dozerAI->setBuildSubTask( DOZER_MOVING_TO_BUILD_DOCK_LOCATION );

			}  // end if
			
			// if we're moving to the build dock location, when we become idle we are there
			if( dozerAI->getBuildSubTask() == DOZER_MOVING_TO_BUILD_DOCK_LOCATION )
			{
				if( ai->isIdle() )
				{
					dozerAI->setBuildSubTask( DOZER_DO_BUILD_AT_DOCK );
					// Get the audio sound and start playing the construction sound (get the sound
					// from the building itself)
					dozerAI->startBuildingSound( goalObject->getTemplate()->getPerUnitSound( "UnderConstruction" ), goalObject->getID() );
				}
			}  // end if

			// only do the build if we've moved into the dock position
			if( dozerAI->getBuildSubTask() == DOZER_DO_BUILD_AT_DOCK )
			{

				// the builder is now actively constructing something
				dozer->setModelConditionState( MODELCONDITION_ACTIVELY_CONSTRUCTING );
				

				// increase the construction percent of the goal object
				Int framesToBuild = goalObject->getTemplate()->calcTimeToBuild( dozer->getControllingPlayer() );
				Real percentProgressThisFrame = 100.0f / framesToBuild;
				goalObject->setConstructionPercent( goalObject->getConstructionPercent() + 
																						percentProgressThisFrame );

				//
				// every time we construct a piece of the goal object, the goal object gets a little
				// bit o health, note that we're bypassing the regular healing/damage methods
				// here and going straight for the change of the health
				//
				BodyModuleInterface *body = goalObject->getBodyModule();
				body->internalChangeHealth( body->getMaxHealth() / INT_TO_REAL( framesToBuild ) );

				//
				// since we've just actually contributed a "piece" to this building, we'll say that
				// we are now the producer and the builder
				//
				goalObject->setProducer( dozer );
				goalObject->setBuilder( dozer );

				// check for construction complete
				if( goalObject->getConstructionPercent() >= 100.0f )
				{

					// clear the under construction status
					goalObject->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_UNDER_CONSTRUCTION ) );
					goalObject->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_RECONSTRUCTING ) );

					// stop playing the construction sound!
					dozerAI->finishBuildingSound();

					// object will now be idle instead of in one of the construction actions
					goalObject->clearModelConditionFlags(
						MAKE_MODELCONDITION_MASK3(MODELCONDITION_AWAITING_CONSTRUCTION, MODELCONDITION_PARTIALLY_CONSTRUCTED, MODELCONDITION_ACTIVELY_BEING_CONSTRUCTED));

					// set the construction at the 100% enum value
					goalObject->setConstructionPercent( CONSTRUCTION_COMPLETE );

					//
					// now that we're done with construction we evaluate the visual condition of
					// the object since while under construction visual changes due to damage state
					// do not occur.  Note that is was important to call this after we cleared the
					// model conditions involving construction because part of this evaluation
					// is to auto populate the model with particle systems when special named
					// bones for the current given state are provided
					//
					body->evaluateVisualCondition();

					// this object now has energy influence in the player
					Player *player = goalObject->getControllingPlayer();
					if( player )
					{

						// notification for build completeion
						player->onStructureConstructionComplete( dozer, goalObject, dozerAI->getIsRebuild() );

						player->getAcademyStats()->recordProduction( goalObject, dozer );

						//
						// Now onCreates were called at construction start.  Now at finish is when we
						// want the Game side of OnCreate
						//
						for (BehaviorModule** m = goalObject->getBehaviorModules(); *m; ++m)
						{
							CreateModuleInterface* create = (*m)->getCreate();
							if (!create)
								continue;
							create->onBuildComplete();
						}

					}  // end if

					// Creation is another valid and essential time to call this.  This building now Looks.
					goalObject->handlePartitionCellMaintenance();
					
					// UnderConstruction just cleared, so update our upgrades
					goalObject->updateUpgradeModules();

					// this object how has influence in the controlling players' tech tree
					/// @todo need to write this

					// do some UI stuff for the constrolling player
					if( dozer->isLocallyControlled() )
					{

						// message the the building player
						UnicodeString format = TheGameText->fetch( "DOZER:ConstructionComplete" );
						UnicodeString objectName = goalObject->getTemplate()->getDisplayName();
						if( objectName.isEmpty() )
						{
							UnicodeString format = TheGameText->fetch( "INI:MissingDisplayName" );

							objectName.format( format, goalObject->getTemplate()->getName().str() );

						}  // end if

						UnicodeString msg;
						msg.format( format.str(), objectName.str() );
						TheInGameUI->message( msg );

						AudioEventRTS audio = *dozer->getTemplate()->getVoiceTaskComplete();
						audio.setObjectID(dozer->getID());
						TheAudio->addAudioEvent(&audio);

						/// make radar neat-o attention grabber event at build location
						TheRadar->createEvent( goalObject->getPosition(), RADAR_EVENT_CONSTRUCTION );

					}  // end if

					// this will allow us to exit the state machine with success
					complete = TRUE;
	
					// move off to the end dock position if present
					const Coord3D *endPos = dozerAI->getDockPoint( m_task, DOZER_DOCK_POINT_END );
					Coord3D pos = *dozer->getPosition();
					if( endPos ) {
						pos = *endPos;
					}
					// Our goal may be inside a building, if we are packing them in tight, so try adjusting. jba.
					TheAI->pathfinder()->adjustToPossibleDestination(dozer, ai->getLocomotorSet(), &pos);
					ai->aiMoveToPosition( &pos, CMD_FROM_AI );

				}  // end if

			}  // end if

			break;

		}  // end build

		//---------------------------------------------------------------------------------------------
		case DOZER_TASK_REPAIR:
		{
			BodyModuleInterface *body = goalObject->getBodyModule();

			// check for fully "repaired"
			if( body->getHealth() == body->getMaxHealth() )
			{

				// issue repair complete message, we might want to remove this later cause it could be annoying
				TheInGameUI->message( "DOZER:RepairComplete" );

				// we're now complete
				complete = TRUE;

			}  // end if
			else
			{
				Bool canHeal = TRUE;

				// if we are repairing a bridge, create scaffolding over the bridge if we need to
				if( goalObject->isKindOf( KINDOF_BRIDGE_TOWER ) )
					dozerAI->createBridgeScaffolding( goalObject );

				// the builder is now actively repairing something, we'll borrow the constructing animation
				dozer->setModelConditionState( MODELCONDITION_ACTIVELY_CONSTRUCTING );

				//
				// when repairing bridges, we cannot actually do any repairing until the
				// scaffolding is extended and all the way complete
				//
				if( goalObject->isKindOf( KINDOF_BRIDGE_TOWER ) )
				{
					BridgeTowerBehaviorInterface *btbi = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( goalObject );
					DEBUG_ASSERTCRASH( btbi, ("Unable to find bridge tower interface") );
					Object *bridgeObject = TheGameLogic->findObjectByID( btbi->getBridgeID() );
					DEBUG_ASSERTCRASH( bridgeObject, ("Unable to find bridge center object") );
					BridgeBehaviorInterface *bbi = BridgeBehavior::getBridgeBehaviorInterfaceFromObject( bridgeObject );
					DEBUG_ASSERTCRASH( bbi, ("Unable to find bridge interface from tower goal object during repair") );

					if( bbi->isScaffoldInMotion() == TRUE )
						canHeal = FALSE;

				}  // end if

				// do healing
				if( canHeal )
				{

					// figure out how much health we will restore this frame
					Real health = body->getMaxHealth() * dozerAI->getRepairHealthPerSecond() /
												LOGICFRAMES_PER_SECOND;

					// try to give it a little bit-o-health
					if ( ! goalObject->attemptHealingFromSoleBenefactor(health, dozer, 2) )//this frame and the next
					{
						// goalObject->setStatus( OBJECT_STATUS_UNDERGOING_REPAIR );
						// This bit used to be set way back in DozerAIUpdate::privateRepair(), but it has been outmoded
						// so that several dozers/workers can target the same sick building for repair, but the first one to begin
						// this is done by attemptHealingFromSoleBenefactor() which lets the beneficiary of the healing
						// remember who has been healing it, and will return false to everybody else
						//or if the goalObject is already receiving healing, I must stop, since my healing is getting rejected here
						dozerAI->internalTaskComplete( m_task );
						getMachine()->setGoalObject( NULL );
						return STATE_FAILURE;
					}



				}  // end if

			}  // end else

			// play a repairing sound

			break;

		}  // end repair

		//---------------------------------------------------------------------------------------------
		case DOZER_TASK_FORTIFY:
		{

			/// @todo write me
			break;

		}  // end fortify

		//---------------------------------------------------------------------------------------------
		default:
		{

			DEBUG_CRASH(( "Unknown task for the dozer action do action state" ));
			return STATE_FAILURE;

		}  // end default

	}  // end switch

	// if we're complete with the task we exit success
	if( complete == TRUE )
	{

		// this task is now complete, remove it from dozer consideration
		dozerAI->internalTaskComplete( m_task );

		// to be clean get rid of the goal object we set
		getMachine()->setGoalObject( NULL );
 

		getMachineOwner()->setWeaponSetFlag(WEAPONSET_MINE_CLEARING_DETAIL);//maybe go clear some mines, if I feel like it
		// we're done
		return STATE_SUCCESS;

	}  // end if

	// just continue sitting here for a while
	getMachineOwner()->clearWeaponSetFlag(WEAPONSET_MINE_CLEARING_DETAIL);// no mine clearing fun while I'm on the job

	return STATE_CONTINUE;

}  // end update

//-------------------------------------------------------------------------------------------------
/** The Dozer action state machine */
//-------------------------------------------------------------------------------------------------
class DozerActionStateMachine : public StateMachine
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( DozerActionStateMachine, "DozerActionStateMachine" );

public:

  DozerActionStateMachine( Object *owner, DozerTask task );
	// virtual destructor prototypes provided by memory pool object

protected:
	// snapshot interface
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();

protected:

	DozerTask m_task;				///< the task of this action state machine

};
EMPTY_DTOR(DozerActionStateMachine)

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DozerActionStateMachine::DozerActionStateMachine( Object *owner, DozerTask task ) : 
												 StateMachine( owner, "DozerActionStateMachine" )
{

	// initialize our task
	m_task = task;

	// order matters: first state is the default state.
	defineState( DOZER_ACTION_PICK_ACTION_POS, newInstance(DozerActionPickActionPosState)( this, task ), DOZER_ACTION_MOVE_TO_ACTION_POS, EXIT_MACHINE_WITH_FAILURE );
	defineState( DOZER_ACTION_MOVE_TO_ACTION_POS, newInstance(DozerActionMoveToActionPosState)( this, task ), DOZER_ACTION_DO_ACTION, DOZER_ACTION_PICK_ACTION_POS );
	defineState( DOZER_ACTION_DO_ACTION, newInstance(DozerActionDoActionState)( this, task ), EXIT_MACHINE_WITH_SUCCESS, EXIT_MACHINE_WITH_FAILURE );
}


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerActionStateMachine::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DozerActionStateMachine::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferUser(&m_task, sizeof(m_task));
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerActionStateMachine::loadPostProcess( void )
{
}  // end loadPostProcess


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static Object *findObjectToRepair( Object *dozer )
{

	// sanity
	if( dozer == NULL )
		return  NULL;
	if( !dozer->getAIUpdateInterface() )
	{
		return NULL; 
	}
	const DozerAIInterface *dozerAI = dozer->getAIUpdateInterface()->getDozerAIInterface();

	PartitionFilterSamePlayer filter1( dozer->getControllingPlayer() );
	PartitionFilterAcceptByKindOf filter2( MAKE_KINDOF_MASK( KINDOF_STRUCTURE ),
																				 KINDOFMASK_NONE );
	PartitionFilterSameMapStatus filterMapStatus(dozer);
	PartitionFilter *filters[] = { &filter1, &filter2, &filterMapStatus, NULL };
	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( dozer->getPosition(),
																																		 dozerAI->getBoredRange(),
																																		 FROM_CENTER_2D,
																																		 filters );

	MemoryPoolObjectHolder hold( iter );
	Object *obj;
	Object *closestRepairTarget = NULL;
	Real closestRepairTargetDistSqr = 0.0f;
	for( obj = iter->first(); obj; obj = iter->next() )
	{

		// ignore objects we cant repair
		if( TheActionManager->canRepairObject( dozer, obj, CMD_FROM_AI ) == FALSE )
			continue;

		// target the closest valid repair target
		if( closestRepairTarget == NULL )
		{

			closestRepairTarget = obj;
			closestRepairTargetDistSqr = ThePartitionManager->getDistanceSquared( dozer, obj, FROM_CENTER_2D );

		}  // end if
		else
		{

			// only use this command center if it's closer than the last one we found
			Real distSqr = ThePartitionManager->getDistanceSquared( dozer, obj, FROM_CENTER_2D );
			if( distSqr < closestRepairTargetDistSqr )
			{
				
				closestRepairTarget = obj;
				closestRepairTargetDistSqr = distSqr;

			}  // end if

		}  // end else

	}  // end for obj

	return closestRepairTarget;

}  // end findObjectToRepair

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static Object *findMine( Object *dozer )
{

	// sanity
	if( dozer == NULL )
		return  NULL;
	if( !dozer->getAIUpdateInterface() )
	{
		return NULL; 
	}
	const DozerAIInterface *dozerAI = dozer->getAIUpdateInterface()->getDozerAIInterface();

	// srj sez: only clear enemy or neutal mines. clearing allied mines (ie, OURS) is really dumb.
	// (and no, PossibleToAttack won't necessarily filter these out.)
	PartitionFilterRelationship	filterTeam(dozer, PartitionFilterRelationship::ALLOW_ENEMIES | PartitionFilterRelationship::ALLOW_NEUTRAL);
	PartitionFilterPossibleToAttack filterAttack(ATTACK_NEW_TARGET, dozer, CMD_FROM_DOZER);
	PartitionFilterSameMapStatus filterMapStatus(dozer);
	PartitionFilter *filters[] = { &filterTeam, &filterAttack, &filterMapStatus, NULL };
	Object* mine = ThePartitionManager->getClosestObject(dozer, dozerAI->getBoredRange(), FROM_CENTER_2D, filters);

	return mine;
}  // end findMine

//-------------------------------------------------------------------------------------------------
/** Available primary Dozer states */
//-------------------------------------------------------------------------------------------------
enum
{
	DOZER_PRIMARY_IDLE,					///< dozer is idle
	DOZER_PRIMARY_BUILD,				///< dozer is building a structure for the player
	DOZER_PRIMARY_REPAIR,				///< dozer is repairing something
	DOZER_PRIMARY_FORTIFY,			///< dozer is fortifying a civilian building, making it stronger
	DOZER_PRIMARY_GO_HOME,			///< dozer has nothing else to do so it will return a command center
};

//-------------------------------------------------------------------------------------------------
/** Dozer primary idle state */
//-------------------------------------------------------------------------------------------------
class DozerPrimaryIdleState : public State
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DozerPrimaryIdleState, "DozerPrimaryIdleState")		

public:

	DozerPrimaryIdleState( StateMachine *machine ) : State( machine, "DozerPrimaryIdleState" ) 
	{
		m_idleTooLongTimestamp = 0;
		m_idlePlayerNumber = 0;
		m_isMarkedAsIdle   = FALSE;
	}
	virtual StateReturnType update( void );
	virtual StateReturnType onEnter( void );
	virtual void onExit( StateExitType status );

protected:
	// snapshot interface
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();

protected:

	UnsignedInt m_idleTooLongTimestamp;		///< when this is more than our idle too long time we try to do something about it
	Int m_idlePlayerNumber;				///< Remeber what list we were added to.
	Bool m_isMarkedAsIdle;
};
EMPTY_DTOR(DozerPrimaryIdleState)

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerPrimaryIdleState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DozerPrimaryIdleState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferUnsignedInt(&m_idleTooLongTimestamp);
	xfer->xferInt(&m_idlePlayerNumber);
	xfer->xferBool(&m_isMarkedAsIdle);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerPrimaryIdleState::loadPostProcess( void )
{
}  // end loadPostProcess


//-------------------------------------------------------------------------------------------------
/** Upon entering the dozer primary idle state */
//-------------------------------------------------------------------------------------------------
StateReturnType DozerPrimaryIdleState::onEnter( void )
{

	//
	// upon entering this state we begin tracking how long we're idle for to try to do something
	// about it every once in a while
	//
	m_idleTooLongTimestamp = TheGameLogic->getFrame();
		//if(TheGameLogic->getFrame() > m_idleNotifyTimestamp + NOTIFY_AFTER_TIME)
	m_isMarkedAsIdle = FALSE;

	getMachineOwner()->setWeaponSetFlag(WEAPONSET_MINE_CLEARING_DETAIL);//maybe go clear some mines, if I feel like it

	return STATE_CONTINUE;

}  // end onEnter

//-------------------------------------------------------------------------------------------------
/** Upon exiting the dozer primary idle state */
//-------------------------------------------------------------------------------------------------
void DozerPrimaryIdleState::onExit( StateExitType status )
{
	if(m_isMarkedAsIdle)
	{
		TheInGameUI->removeIdleWorker(getMachineOwner(), m_idlePlayerNumber);
		m_idlePlayerNumber = -1;
		m_isMarkedAsIdle = FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
/** Dozer idle behavior */
//-------------------------------------------------------------------------------------------------
StateReturnType DozerPrimaryIdleState::update( void )
{
	Object *dozer = getMachineOwner();
	AIUpdateInterface *ai = dozer->getAIUpdateInterface();
	if( !ai )
	{
		return STATE_FAILURE;
	}
	DozerAIInterface *dozerAI = ai->getDozerAIInterface();
	if( !dozerAI )
	{
		return STATE_FAILURE;
	}

	//
	// These are to add into the IngameUI idle worker button thingy
	// we don't want to add in if we're already in the list or if
	// we're "Effectivly dead"
	//
	if( ai->isIdle() && !m_isMarkedAsIdle && !dozer->isEffectivelyDead())
	{
		m_idlePlayerNumber = dozer->getControllingPlayer()->getPlayerIndex();
		TheInGameUI->addIdleWorker(getMachineOwner());
		m_isMarkedAsIdle = TRUE;
		getMachineOwner()->setWeaponSetFlag(WEAPONSET_MINE_CLEARING_DETAIL);//maybe go clear some mines, if I feel like it

	}
	if( m_isMarkedAsIdle && (!ai->isIdle() || dozer->isEffectivelyDead()))
	{
		TheInGameUI->removeIdleWorker(getMachineOwner(), m_idlePlayerNumber);
		m_idlePlayerNumber = -1;
		m_isMarkedAsIdle = FALSE;
	}

	//
	// if our actual real AI state machine is not idle we're doing something so let's reset
	// our idle too long timestamp
	//
	if( !ai->isIdle() )
		m_idleTooLongTimestamp = TheGameLogic->getFrame();
	
	//
	// if we're just sitting here in idle, and there is no task pending ... after so long we'll
	// try to go to the nearest command center
	//
	if( TheGameLogic->getFrame() - m_idleTooLongTimestamp > dozerAI->getBoredTime() &&
			dozerAI->isAnyTaskPending() == FALSE )
	{

		//
		// to prevent us from doing this potentially expensive logic to find objects every frame
		// we'll only try it every once in a while so set our idle too long timestamp to the
		// current frame
		//
		m_idleTooLongTimestamp = TheGameLogic->getFrame();

		// try to find something around us that we can repair
		Object *repairTarget = findObjectToRepair( dozer );
		if( repairTarget )
		{

			// issue the command
			ai->aiRepair( repairTarget, CMD_FROM_AI );

			//
			// in theory we would need to "interrupt" whatever it is the Dozer is doing now, but
			// we know we're in the idle state doing nothing so we don't really need to
			//

		} else {
			getMachineOwner()->setWeaponSetFlag(WEAPONSET_MINE_CLEARING_DETAIL);//maybe go clear some mines, if I feel like it
			Object *mine = findMine(dozer);
			if (mine!=NULL) {
				ai->aiAttackObject( mine, 1, CMD_FROM_DOZER);
			}
		}

					
	}  // end if

	return STATE_CONTINUE;

}  // end update

//-------------------------------------------------------------------------------------------------
/** Dozer action state */
//-------------------------------------------------------------------------------------------------
class DozerActionState : public State
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DozerActionState, "DozerActionState")		

public:

	DozerActionState( StateMachine *machine, DozerTask task ) : State( machine, "DozerActionState" )
	{
		m_task = task;
		m_actionMachine = newInstance(DozerActionStateMachine)( getMachineOwner(), task );
		m_actionMachine->initDefaultState();
	}
	/* 
		Note that we DON'T use CONVERT_SLEEP_TO_CONTINUE; since we're not doing anything else
		interesting in update, we can sleep when this machine sleeps 
	*/
	virtual StateReturnType update( void ) { return m_actionMachine->updateStateMachine(); }

	virtual StateReturnType onEnter( void );
	virtual void onExit( StateExitType status );

protected:
	// snapshot interface
	virtual void crc( Xfer *xfer );
	virtual void xfer( Xfer *xfer );
	virtual void loadPostProcess();

protected:

	DozerTask m_task;									// task for this state (and therefore sub-machine too)
	StateMachine *m_actionMachine;

};
inline DozerActionState::~DozerActionState( void ) { if (m_actionMachine) deleteInstance(m_actionMachine); }

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerActionState::crc( Xfer *xfer )
{
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DozerActionState::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

	xfer->xferUser(&m_task, sizeof(m_task));
	xfer->xferSnapshot(m_actionMachine);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerActionState::loadPostProcess( void )
{
}  // end loadPostProcess


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
StateReturnType DozerActionState::onEnter( void )
{

	// save this as the current action of the dozer
	Object *dozer = getMachineOwner();
	if( !dozer->getAIUpdateInterface() )
	{
		return STATE_FAILURE;
	}
	DozerAIInterface *dozerAI = dozer->getAIUpdateInterface()->getDozerAIInterface();
	dozerAI->setCurrentTask( m_task );

	//
	// we have a machine within this state all it's own ...
	// note that during an onEnter, since the action machine is persistent, anytime we transition
	// into doing an action, we need to reset our state machine to do that action as it may
	// have been left in any state from the previous time we ran it
	//
	m_actionMachine->resetToDefaultState();

	return STATE_CONTINUE;

}  // end onEnter

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void DozerActionState::onExit( StateExitType status )
{

	// save the current action of the dozer as none
	Object *dozer = getMachineOwner();
	if( !dozer->getAIUpdateInterface() )
	{
		return;
	}
	DozerAIInterface *dozerAI = dozer->getAIUpdateInterface()->getDozerAIInterface();
	dozerAI->setCurrentTask( DOZER_TASK_INVALID );

}  // end onExit

//-------------------------------------------------------------------------------------------------
/** Dozer primary going home state */
//-------------------------------------------------------------------------------------------------
class DozerPrimaryGoingHomeState : public State
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DozerPrimaryGoingHomeState, "DozerPrimaryGoingHomeState")		

protected:
	// snapshot interface	 STUBBED no member vars
	virtual void crc( Xfer *xfer ){};
	virtual void xfer( Xfer *xfer ){};
	virtual void loadPostProcess(){};

public:

	DozerPrimaryGoingHomeState( StateMachine *machine ) : State( machine, "DozerPrimaryGoingHomeState" ) { }
	virtual StateReturnType update( void ) { return STATE_FAILURE; }

};
EMPTY_DTOR(DozerPrimaryGoingHomeState)

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DozerPrimaryStateMachine::DozerPrimaryStateMachine( Object *owner ) : StateMachine( owner, "DozerPrimaryStateMachine" )
{
	static const StateConditionInfo idleConditions[] = 
	{
		StateConditionInfo(isBuildMostImportant, DOZER_PRIMARY_BUILD, NULL),
		StateConditionInfo(isRepairMostImportant, DOZER_PRIMARY_REPAIR, NULL),
		StateConditionInfo(isFortifyMostImportant, DOZER_PRIMARY_FORTIFY, NULL),
		StateConditionInfo(NULL, NULL, NULL)	// keep last
	};

	// order matters: first state is the default state.
	defineState( DOZER_PRIMARY_IDLE, newInstance(DozerPrimaryIdleState)( this ), INVALID_STATE_ID, INVALID_STATE_ID, idleConditions );
	defineState( DOZER_PRIMARY_BUILD, newInstance(DozerActionState)( this, DOZER_TASK_BUILD ), DOZER_PRIMARY_IDLE, DOZER_PRIMARY_IDLE );
	defineState( DOZER_PRIMARY_REPAIR, newInstance(DozerActionState)( this, DOZER_TASK_REPAIR ), DOZER_PRIMARY_IDLE, DOZER_PRIMARY_IDLE );
	defineState( DOZER_PRIMARY_FORTIFY, newInstance(DozerActionState)( this, DOZER_TASK_FORTIFY ), DOZER_PRIMARY_IDLE, DOZER_PRIMARY_IDLE );
	defineState( DOZER_PRIMARY_GO_HOME, newInstance(DozerPrimaryGoingHomeState)( this ), DOZER_PRIMARY_IDLE, DOZER_PRIMARY_IDLE );

}  // end DozerPrimaryStateMachine

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DozerPrimaryStateMachine::~DozerPrimaryStateMachine( void )
{

}  // end ~DozerPrimaryStateMachine

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerPrimaryStateMachine::crc( Xfer *xfer )
{
	StateMachine::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DozerPrimaryStateMachine::xfer( Xfer *xfer )
{
	XferVersion cv = 1;	
	XferVersion v = cv; 
	xfer->xferVersion( &v, cv );

	StateMachine::xfer(xfer);
}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerPrimaryStateMachine::loadPostProcess( void )
{
	StateMachine::loadPostProcess();
}  // end loadPostProcess

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool DozerPrimaryStateMachine::isBuildMostImportant( State *thisState, void* userData )
{
	Object *dozer = thisState->getMachineOwner();
	AIUpdateInterface *ai = dozer->getAIUpdateInterface();
	if( !ai )
	{
		return FALSE;
	}
	DozerAIInterface *dozerAI = ai->getDozerAIInterface();
	if( !dozerAI )
	{
		return FALSE;
	}

	if( !ai->isIdle() )
		return FALSE;  // busy doing something else

	// if the most important task is us then return true
	DozerTask task = dozerAI->getMostRecentCommand();
	return task == DOZER_TASK_BUILD;

}  // end isBuildMostImportant

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool DozerPrimaryStateMachine::isRepairMostImportant( State *thisState, void* userData )
{
	Object *dozer = thisState->getMachineOwner();
	AIUpdateInterface *ai = dozer->getAIUpdateInterface();
	if( !ai )
	{
		return false;
	}
	DozerAIInterface *dozerAI = ai->getDozerAIInterface();
	if( !dozerAI )
	{
		return false;
	}

	if( !ai->isIdle() )
		return FALSE;  // busy doing something else

	// if the most important task is us then return true
	DozerTask task = dozerAI->getMostRecentCommand();
	return task == DOZER_TASK_REPAIR;

}  // end isRepairMostImportant

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool DozerPrimaryStateMachine::isFortifyMostImportant( State *thisState, void* userData )
{
	Object *dozer = thisState->getMachineOwner();
	AIUpdateInterface *ai = dozer->getAIUpdateInterface();
	if( !ai )
	{
		return false;
	}
	DozerAIInterface *dozerAI = ai->getDozerAIInterface();
	if( !dozerAI )
	{
		return false;
	}

	if( !ai->isIdle() )
		return FALSE;  // busy doing something else

	// if the most important task is us then return true
	DozerTask task = dozerAI->getMostRecentCommand();
	return task == DOZER_TASK_FORTIFY;

}  // end isFortifyMostImportat

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
DozerAIUpdateModuleData::DozerAIUpdateModuleData( void )
{

	m_repairHealthPercentPerSecond = 0.0f;
	m_boredTime = 0.0f;
	m_boredRange = 0.0f;

}  // end DozerAIUpdateModuleData

// ------------------------------------------------------------------------------------------------
void DozerAIUpdateModuleData::buildFieldParse( MultiIniFieldParse& p)
{
  AIUpdateModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] = 
	{
		{ "RepairHealthPercentPerSecond",	INI::parsePercentToReal,	NULL, offsetof( DozerAIUpdateModuleData, m_repairHealthPercentPerSecond ) },
		{ "BoredTime",										INI::parseDurationReal,		NULL, offsetof( DozerAIUpdateModuleData, m_boredTime ) },
		{ "BoredRange",										INI::parseReal,						NULL, offsetof( DozerAIUpdateModuleData, m_boredRange ) },
		{ 0, 0, 0, 0 }
	};

  p.add( dataFieldParse );

}  // end buildFieldParse

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DozerAIUpdate::DozerAIUpdate( Thing *thing, const ModuleData* moduleData ) : 
							 AIUpdateInterface( thing, moduleData )
               
{
	Int i, j;

	for( i = 0; i < DOZER_NUM_TASKS; i++ )
	{

		m_task[ i ].m_targetObjectID = INVALID_ID;
		m_task[ i ].m_taskOrderFrame = 0;

		for( j = 0; j < DOZER_NUM_DOCK_POINTS; j++ )
		{

			m_dockPoint[ i ][ j ].valid = FALSE;
			m_dockPoint[ i ][ j ].location.zero();

		}  // end for j

	}  // end for i
	m_currentTask = DOZER_TASK_INVALID;

	m_buildSubTask = DOZER_SELECT_BUILD_DOCK_LOCATION;  // irrelavant, but I want non-garbage value

	//
	// initialize the dozer machine to NULL, we want to do this and create it during the update
	// implementation because at this point we don't have the object all setup
	//
	m_dozerMachine = NULL;

	createMachines();
}  // end DozerAIUpdate

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DozerAIUpdate::~DozerAIUpdate( void )
{

	// delete our behavior state machine
	if( m_dozerMachine )
		deleteInstance(m_dozerMachine);

	// no orders
	for( Int i = 0; i < DOZER_NUM_TASKS; i++ )
	{

		m_task[ i ].m_targetObjectID = INVALID_ID;
		m_task[ i ].m_taskOrderFrame = 0;

	}  // end for i

}  // end ~DozerAIUpdate

// ------------------------------------------------------------------------------------------------
/** Create any sub machines we need to do all our behavior */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::createMachines( void )
{

	if( m_dozerMachine == NULL )
	{

		m_dozerMachine = newInstance(DozerPrimaryStateMachine)( getObject() );
		m_dozerMachine->initDefaultState();

	}  // end if

}  // end createMachines

// ------------------------------------------------------------------------------------------------
/** Create the bridge scaffolding if necessary for the bridge that is attached to this tower */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::createBridgeScaffolding( Object *bridgeTower )
{

	// sanity
	if( bridgeTower == NULL )
		return;

	// get the bridge behavior interface from the bridge object that this tower is a part of
	BridgeTowerBehaviorInterface *btbi = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( bridgeTower );
	if( btbi == NULL )
		return;
	Object *bridgeObject = TheGameLogic->findObjectByID( btbi->getBridgeID() );
	if( bridgeObject == NULL )
		return;
	BridgeBehaviorInterface *bbi = BridgeBehavior::getBridgeBehaviorInterfaceFromObject( bridgeObject );
	if( bbi == NULL )
		return;

	// tell the bridge to create scaffolding if necessary
	bbi->createScaffolding();

}  // end createBridgeScaffolding

// ------------------------------------------------------------------------------------------------
/** Remove the bridge scaffolding from the bridge object that is attached to this tower */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::removeBridgeScaffolding( Object *bridgeTower )
{

	// sanity
	if( bridgeTower == NULL )
		return;

	// get the bridge behavior interface from the bridge object that this tower is a part of
	BridgeTowerBehaviorInterface *btbi = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( bridgeTower );
	if( btbi == NULL )
		return;
	Object *bridgeObject = TheGameLogic->findObjectByID( btbi->getBridgeID() );
	if( bridgeObject == NULL )
		return;
	BridgeBehaviorInterface *bbi = BridgeBehavior::getBridgeBehaviorInterfaceFromObject( bridgeObject );
	if( bbi == NULL )
		return;

	// tell the bridge to end any scaffolding from repairing
	bbi->removeScaffolding();

}  // end removeBridgeScaffolding

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime DozerAIUpdate::update( void )
{

	//
	// NOTE: Any changes to DozerAIUpdate::* you probably want to reflect and copy into
	// WorkerAIUPdate:* as well ... sigh
	//

	//
	// now that we're really executing we have all the necessary object modules in place to
	// correctly create a state machine and set the default state
	//
	createMachines();

	// set us as being to able to move with super precision off grid locations
 /*
	if( getCurLocomotor() )	 {
			getCurLocomotor()->setUltraAccurate( TRUE );
			getCurLocomotor()->setAllowInvalidPosition(TRUE);
	}*/
	
	// extend the normal AI system
	UpdateSleepTime result;
	result = AIUpdateInterface::update();

	// do nothing if we're dead
	///@todo shouldn't this be at a higher level?
	if( getObject()->isEffectivelyDead() )
		return UPDATE_SLEEP_NONE;

	// get and validate our current task
	DozerTask currentTask = getCurrentTask();
	if( currentTask != DOZER_TASK_INVALID )
	{
		

		ObjectID taskTarget = getTaskTarget( currentTask );
		Object *targetObject = TheGameLogic->findObjectByID( taskTarget );
		Bool invalidTask = FALSE;

		// validate the task and the target
		if( currentTask == DOZER_TASK_REPAIR &&
				TheActionManager->canRepairObject( getObject(), targetObject, getLastCommandSource() ) == FALSE )
			invalidTask = TRUE;
		
		// cancel the task if it's now invalid
		if( invalidTask == TRUE )
			cancelTask( currentTask );

	}  // end if
	else
		getObject()->setWeaponSetFlag(WEAPONSET_MINE_CLEARING_DETAIL);//maybe go clear some mines, if I feel like it

	// run our own state machine
	m_dozerMachine->updateStateMachine();

	return UPDATE_SLEEP_NONE;
		
}  // end update

//-------------------------------------------------------------------------------------------------
/** The entry point of a construct command to the Dozer */
//-------------------------------------------------------------------------------------------------
Object *DozerAIUpdate::construct( const ThingTemplate *what, 
																	const Coord3D *pos, 
																	Real angle, 
																	Player *owningPlayer,
																	Bool isRebuild )
{

	// !!! NOTE: If you modify this you must modify the worker too !!!
	// !!! Graham: Please please please have inspiration for how to *not* duplicate this code

	// create our machines if they don't yet exist
	///@todo make 'construct' a real AI command and you won't need a special case
	m_isRebuild = isRebuild;

	createMachines();

	// sanity
	if( what == NULL || pos == NULL || owningPlayer == NULL )
		return NULL;

	// sanity
	DEBUG_ASSERTCRASH( getObject()->getControllingPlayer() == owningPlayer,
										 ("Dozer::Construct - The controlling player of the Dozer is not the owning player passed in") );
	
	// if we're not rebuilding, we have a few checks to pass first for sanity
	if( isRebuild == FALSE )
	{

		// AI has weaker restriction on building
		Bool dozerIsAI = owningPlayer->getPlayerType() == PLAYER_COMPUTER;
		if( dozerIsAI )
		{

			// Just build it.  The ai will validate, or cheat.  jba.
			
		}  // end if
		else
		{

			// make sure the player is capable of building this
			if( TheBuildAssistant->canMakeUnit( getObject(), what ) != CANMAKE_OK)
				return NULL;

			// validate the the position to build at is valid
			if( TheBuildAssistant->isLocationLegalToBuild( pos, what, angle,
																										 BuildAssistant::TERRAIN_RESTRICTIONS |
																										 BuildAssistant::CLEAR_PATH |
																										 BuildAssistant::NO_OBJECT_OVERLAP |
																										 BuildAssistant::SHROUD_REVEALED,
																										 getObject(), NULL ) != LBC_OK )
				return NULL;

		}  // end else

	}  // end if

	//
	// what will our initial status bits be, it is important to do this early
	// before the hooks add/subtract power from a player are executed
	//
	ObjectStatusMaskType statusBits = MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_UNDER_CONSTRUCTION );
	if( isRebuild )
		statusBits.set( OBJECT_STATUS_RECONSTRUCTING );

	// create an object at the destination location
	Object *obj = TheThingFactory->newObject( what, owningPlayer->getDefaultTeam(), statusBits );

	// even though we haven't actually built anything yet, this keeps things tidy
	obj->setProducer( getObject() );
	obj->setBuilder( getObject() );

	// take the required money away from the player
	if( isRebuild == FALSE )
	{
		Money *money = owningPlayer->getMoney();

		money->withdraw( what->calcCostToBuild( owningPlayer ) );

	}  // end if	

	// initialize object
	obj->setPosition( pos );
	obj->setOrientation( angle );

	// Flatten the terrain underneath the object, then adjust to the flattened height. jba.
	TheTerrainLogic->flattenTerrain(obj);
	Coord3D adjustedPos = *pos;
	adjustedPos.z = TheTerrainLogic->getGroundHeight(pos->x, pos->y);
	obj->setPosition(&adjustedPos);
	
	// Note - very important that we add to map AFTER we flatten terrain. jba.
	TheAI->pathfinder()->addObjectToPathfindMap( obj );
	// "callback" event for structure created (note that it's not yet "complete")
	owningPlayer->onStructureCreated( getObject(), obj );

	// set a construction percent for the new object to zero and a status for under construction
	obj->setConstructionPercent( 0.0 );

	// newly constructed objects start at one hit point
	BodyModuleInterface *body = obj->getBodyModule();
	body->internalChangeHealth( -body->getHealth() + 1.0f );

	// set the model action state to awaiting construction
	obj->clearAndSetModelConditionFlags(
		MAKE_MODELCONDITION_MASK2(MODELCONDITION_PARTIALLY_CONSTRUCTED, MODELCONDITION_ACTIVELY_BEING_CONSTRUCTED), 
		MAKE_MODELCONDITION_MASK(MODELCONDITION_AWAITING_CONSTRUCTION)
	);

	// we have a construction pending
	newTask( DOZER_TASK_BUILD, obj );

	return obj;
				
}  // end construct

// ------------------------------------------------------------------------------------------------
/** Given our current task and repair target, can we accept this as a new repair target */
// ------------------------------------------------------------------------------------------------
Bool DozerAIUpdate::canAcceptNewRepair( Object *obj )
{

	// sanity
	if( obj == NULL )
		return FALSE;

	// if we're not repairing right now, we don't have any accept restrictions
	if( getCurrentTask() != DOZER_TASK_REPAIR )
		return TRUE;

	// get current repair target
	Object *currentRepair = TheGameLogic->findObjectByID( m_task[ DOZER_TASK_REPAIR ].m_targetObjectID );

	if( currentRepair )
	{

		// check for same object
		if( currentRepair == obj )
			return FALSE;

		// check for repairing any tower on the same bridge
		if( currentRepair->isKindOf( KINDOF_BRIDGE_TOWER ) && 
				obj->isKindOf( KINDOF_BRIDGE_TOWER ) )
		{
			BridgeTowerBehaviorInterface *currentTowerInterface = NULL;
			BridgeTowerBehaviorInterface *newTowerInterface = NULL;

			currentTowerInterface = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( currentRepair );
			newTowerInterface = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( obj );

			// sanity
			if( currentTowerInterface == NULL || newTowerInterface == NULL )
			{

				DEBUG_CRASH(( "Unable to find bridge tower interface on object" ));
				return FALSE;

			}  // end if

			// if they are part of the same bridge, ignore this repair command
			if( currentTowerInterface->getBridgeID() == newTowerInterface->getBridgeID() )
				return FALSE;

		}  // end if

	}  // end if, currentRepair object exists

	// all is well
	return TRUE;

}  // end canAcceptNewRepair

// ------------------------------------------------------------------------------------------------
/** Issue an order for the Dozer to go repair the target 'obj' */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::privateRepair( Object *obj, CommandSourceType cmdSource )
{
	Object *dozer = getObject();

	// if we are already repairing this target do nothing
	if( canAcceptNewRepair( obj ) == FALSE )
		return;

	// sanity, if we can't repair the object then get out of there
	if( TheActionManager->canRepairObject( dozer, obj, cmdSource ) == FALSE )
		return;

	// if this object is already actively being repaired by another dozertype we won't also try to go repair it
	ObjectID currentRepairer = obj->getSoleHealingBenefactor();
	if( currentRepairer != INVALID_ID && currentRepairer != dozer->getID() )
		return;

	// Bridges have been made indestructible, so these checks were in vain. -- ML

	// if this object is a bridge tower, we need to check the status of the bridge in order
	// to check for a 'duplicate repair'
	//
	//if( obj->isKindOf( KINDOF_BRIDGE_TOWER ) )
	//{
	//	BridgeTowerBehaviorInterface *btbi = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( obj );
	//	DEBUG_ASSERTCRASH( btbi, ("Unable to find bridge tower behavior interface") );
  //
	//	Object *bridge = TheGameLogic->findObjectByID( btbi->getBridgeID() );
	//	DEBUG_ASSERTCRASH( bridge, ("Unable to find bridge object") );
	//	if( BitIsSet( bridge->getStatusBits(), OBJECT_STATUS_UNDERGOING_REPAIR ) == TRUE )
	//		return;
  //
	//}  // end if
	
	
	// for bridges, set the status for the bridge object
	//if( obj->isKindOf( KINDOF_BRIDGE_TOWER ) )
	//{
	//	BridgeTowerBehaviorInterface *btbi = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( obj );
	//	DEBUG_ASSERTCRASH( btbi, ("Unable to find bridge tower behavior interface") );
	//
	//  Object *bridge = TheGameLogic->findObjectByID( btbi->getBridgeID() );
	//	DEBUG_ASSERTCRASH( bridge, ("Unable to find bridge object") );
	//	bridge->setStatus( OBJECT_STATUS_UNDERGOING_REPAIR );
  //
	//}  // end if

	// start the new task
	newTask( DOZER_TASK_REPAIR, obj );

}  // end repair

// ------------------------------------------------------------------------------------------------
/** Resume construction on a building */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::privateResumeConstruction( Object *obj, CommandSourceType cmdSource )
{

	// sanity
	if( obj == NULL )
		return;

	// make sure we can resume construction on this
	if( TheActionManager->canResumeConstructionOf( getObject(), obj, cmdSource ) == FALSE )
		return;

	// start the new task for construction
	newTask( DOZER_TASK_BUILD, obj );

}  // end privateResumeConstruction

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/*static*/ Bool DozerAIUpdate::findGoodBuildOrRepairPosition(const Object* me, const Object* target, Coord3D& positionOut)
{
	// The place we go to build or repair is the closest spot from us to them
	Coord3D ourPosition = *me->getPosition();
	Coord3D theirPosition = *target->getPosition();
	
	Coord3D bestPosition = theirPosition;// This answer is the best, as it includes findPositionAround
	Coord3D workingPosition = theirPosition;// But if findPositionAround fails, we need to say something.
	
	Vector3 offset( ourPosition.x - theirPosition.x, 
									ourPosition.y - theirPosition.y, 
									ourPosition.z - theirPosition.z );
	offset.Normalize();
	// This scaler makes FindPositionAround bias towards our side
	offset = offset * (target->getGeometryInfo().getMajorRadius() / 2);

	workingPosition.x += offset.X;
	workingPosition.y += offset.Y;
	workingPosition.z += offset.Z;

	// this is a little cheesy... the idea is that we can only choose a location that is pretty close
	// in z to the desired one. this prevents us from choosing a space at the bottom of a cliff when
	// the space we want is at the top of the cliff. ideally we should do a funky terrain-zone compare
	// but that isn't well-exposed... (srj)
	const Real MAX_Z_DELTA = 10.0f;

	FindPositionOptions fpOptions;
	fpOptions.minRadius = 0.0f;
	fpOptions.maxRadius = 100.0f;
	fpOptions.sourceToPathToDest = me;// This makes it find a place forWhom can get to.
	if (!me->isUsingAirborneLocomotor())
		fpOptions.maxZDelta = MAX_Z_DELTA;
	if( me->isUsingAirborneLocomotor() )
		fpOptions.ignoreObject = target;// Flyers can ignore stuff, so they can approach right over the target if they want.

	Bool spotFound = ThePartitionManager->findPositionAround( &workingPosition, &fpOptions, &bestPosition );

	positionOut = spotFound ? bestPosition : workingPosition;

	return spotFound;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/*static*/ Object* DozerAIUpdate::findGoodBuildOrRepairPositionAndTarget(Object* me, Object* target, Coord3D& positionOut)
{
	if (target->isKindOf(KINDOF_BRIDGE))
	{
		BridgeBehaviorInterface *bbi = BridgeBehavior::getBridgeBehaviorInterfaceFromObject(target);
		if (bbi)
		{
			AIUpdateInterface* ai = me->getAI();

			// have to repair at a tower.
			Real bestDistSqr = 1e10f;
			Object* bestTower = NULL;
			for (Int i = 0; i < BRIDGE_MAX_TOWERS; ++i)
			{
				Object* tower = TheGameLogic->findObjectByID(bbi->getTowerID((BridgeTowerType)i));
				if( tower )
				{
					Coord3D tmp;
					Bool found = findGoodBuildOrRepairPosition(me, tower, tmp);
					// do isPathAvail against the result of this, NOT the tower pos,
					// since towers are often in cliff cells.
					if (found && ai->isPathAvailable(&tmp))
					{
						Real thisDistSqr = sqr(me->getPosition()->x - tmp.x) + sqr(me->getPosition()->y - tmp.y);
						if (thisDistSqr < bestDistSqr)
						{
							positionOut = tmp;
							bestDistSqr = thisDistSqr;
							bestTower = tower;
						}
					}
				}
			}
			if (bestTower)
				return bestTower;

			DEBUG_CRASH(("should not happen, no reachable tower found"));
			return NULL;
		}
	}

	findGoodBuildOrRepairPosition(me, target, positionOut);
	return target;
}

//-------------------------------------------------------------------------------------------------
/** Issue and order to the dozer */
//-------------------------------------------------------------------------------------------------
void DozerAIUpdate::newTask( DozerTask task, Object *target )
{

	// sanity
	DEBUG_ASSERTCRASH( task >= 0 && task < DOZER_NUM_TASKS, ("Illegal dozer task '%d'", task) );

	// sanity
	if( target == NULL )
		return;

	//
	// special check for the build task, we should never be given more than one of them ...
	// for the other tasks we just forget what we were doing and the new target takes
	// precedence for the task
	//
	if( task == DOZER_TASK_BUILD || task == DOZER_TASK_REPAIR )
	{

		// handle getting two tasks
		if( isTaskPending( task ) == TRUE )
			cancelTask( task );

		// get our object
		Object *me = getObject();

		Coord3D position;
		target = findGoodBuildOrRepairPositionAndTarget(me, target, position);
		if (target == NULL)
			return;	// could happen for some bridges

		//
		// for building, we say that even "thinking" about building or rebuilding an object
		// sets us as the current builder of that object.  this allows any dozers that are 
		// ordered later to resume construction on something to see that somebody is already taking
		// care of it and then they won't be even try to resume a build since we don't allow
		// multiple dozers/workers to double up on construction efforts
		//
		if( task == DOZER_TASK_BUILD )
			target->setBuilder( me );

		m_dockPoint[ task ][ DOZER_DOCK_POINT_START ].valid			= TRUE;
		m_dockPoint[ task ][ DOZER_DOCK_POINT_START ].location	= position;
		m_dockPoint[ task ][ DOZER_DOCK_POINT_ACTION ].valid		= TRUE;
		m_dockPoint[ task ][ DOZER_DOCK_POINT_ACTION ].location = position;
		Coord3D offset;
		offset.set(position.x-target->getPosition()->x, position.y-target->getPosition()->y, 0);
		offset.normalize();
		offset.scale(5*PATHFIND_CELL_SIZE_F);
		position.add(&offset); // move away from the dock point at the end of build.
		m_dockPoint[ task ][ DOZER_DOCK_POINT_END ].valid				= TRUE;
		m_dockPoint[ task ][ DOZER_DOCK_POINT_END ].location		= position;

	}  // end if

	// set the new task target and the frame in which we got this order
	m_task[ task ].m_targetObjectID = target->getID();
	m_task[ task ].m_taskOrderFrame = TheGameLogic->getFrame();

	// reset the dozer behavior so that it can re-evluate which task to continue working on
	m_dozerMachine->resetToDefaultState();

}  // end newTask

//-------------------------------------------------------------------------------------------------
/** Cancel a task and reset the dozer behavior state machine so that it can 
	* re-evaluate what it wants to do if it was working on the task being
	* cancelled */
//-------------------------------------------------------------------------------------------------
void DozerAIUpdate::cancelTask( DozerTask task )
{

	// clear the order
	internalCancelTask( task );

	// reset the machine to we can re-evaluate what we want to do
	m_dozerMachine->resetToDefaultState();

}  // end cancelTask

//-------------------------------------------------------------------------------------------------
/** Is there a given task waiting to be done */
//-------------------------------------------------------------------------------------------------
Bool DozerAIUpdate::isTaskPending( DozerTask task )
{

	// sanity
	DEBUG_ASSERTCRASH( task >= 0 && task < DOZER_NUM_TASKS, ("Illegal dozer task '%d'", task) );

	return m_task[ task ].m_targetObjectID != 0 ? TRUE : FALSE;
		
}  // end isTaskPending

//-------------------------------------------------------------------------------------------------
/** Is there any task pending */
//-------------------------------------------------------------------------------------------------
Bool DozerAIUpdate::isAnyTaskPending( void )
{
	
	for( Int i = 0; i < DOZER_NUM_TASKS; i++ )
		if( isTaskPending( (DozerTask)i ) )
			return TRUE;

	return FALSE;

}  // end isAnyTaskPending

//-------------------------------------------------------------------------------------------------
/** Get the target object of a given task */
//-------------------------------------------------------------------------------------------------
ObjectID DozerAIUpdate::getTaskTarget( DozerTask task )
{

	// sanity
	DEBUG_ASSERTCRASH( task >= 0 && task < DOZER_NUM_TASKS, ("Illegal dozer task '%d'", task) );

	return m_task[ task ].m_targetObjectID;

}  // end getTaskTarget

//-------------------------------------------------------------------------------------------------
/** Set a task as successfully completed */
//-------------------------------------------------------------------------------------------------
void DozerAIUpdate::internalTaskComplete( DozerTask task )
{

	// sanity
	DEBUG_ASSERTCRASH( task >= 0 && task < DOZER_NUM_TASKS, ("Illegal dozer task '%d'", task) );

	// call the single method that gets called for completing and canceling tasks
	internalTaskCompleteOrCancelled( task );

	// remove the info for this task
	m_task[ task ].m_targetObjectID = INVALID_ID;
	m_task[ task ].m_taskOrderFrame = 0;

	// remove dock point info for this task
	for( Int i = 0; i < DOZER_NUM_DOCK_POINTS; i++ )
		m_dockPoint[ task ][ i ].valid = FALSE;

}  // end internalTaskComplete

//-------------------------------------------------------------------------------------------------
/** Clear a task from the Dozer for consideration, we can use this when a goal object becomes
	* invalid/destroyed etc. */
//-------------------------------------------------------------------------------------------------
void DozerAIUpdate::internalCancelTask( DozerTask task )
{

	// sanity
	DEBUG_ASSERTCRASH( task >= 0 && task < DOZER_NUM_TASKS, ("Illegal dozer task '%d'", task) );
	
	if(task < 0 || task >= DOZER_NUM_TASKS)
		return;  //DAMNIT!  You CANNOT assert and then not handle the damn error!  The.  Code.  Must.  Not.  Crash.

	// call the single method that gets called for completing and canceling tasks
	internalTaskCompleteOrCancelled( task );

	// remove the info for this task
	m_task[ task ].m_targetObjectID = INVALID_ID;
	m_task[ task ].m_taskOrderFrame = 0;
	
	// remove dock point info for this task
	for( Int i = 0; i < DOZER_NUM_DOCK_POINTS; i++ )
		m_dockPoint[ task ][ i ].valid = FALSE;
	
	// stop the dozer from moving
	AIUpdateInterface *ai = getObject()->getAIUpdateInterface();
	ai->aiIdle( CMD_FROM_AI );

}  // end internalCancelTask

// ------------------------------------------------------------------------------------------------
/** This method is called whenever a task is completed *OR* cancelled */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::internalTaskCompleteOrCancelled( DozerTask task )
{

	switch( task )
	{

		// --------------------------------------------------------------------------------------------
		case DOZER_TASK_INVALID:	
		{

			break;  // do nothing, this is really no task 

		}  // end invalid
						
		// --------------------------------------------------------------------------------------------
		case DOZER_TASK_BUILD:
		{

			// the builder is no longer actively building something
			getObject()->clearModelConditionState( MODELCONDITION_ACTIVELY_CONSTRUCTING );

			// And the thing we were working on is no longer being actively built

			///@todo This would be correct except that we don't have idle crane animations and it is December.
//			Object* goalObject = TheGameLogic->findObjectByID(m_task[task].m_targetObjectID);
//			if (goalObject != NULL)
//			{
//				goalObject->clearModelConditionState(MODELCONDITION_ACTIVELY_BEING_CONSTRUCTED);
//			}
			break;
		
		}  // end build

		// --------------------------------------------------------------------------------------------
		case DOZER_TASK_REPAIR:
		{

			// the builder is no longer actively repairing something
			getObject()->clearModelConditionState( MODELCONDITION_ACTIVELY_CONSTRUCTING );

			// Bridges have been made indestructible, so the code below was meaningless. -- ML
			// Object *obj = NULL;
			// get object to reapir (if present)
			//obj = TheGameLogic->findObjectByID( m_task[ task ].m_targetObjectID );
			//
			//if( obj )
			//{
      //
			//	
			//	 when we're done repairing bridges, tell the scaffolding to go away and also remove
			//	 the undergoing repair status from the bridge object itself
			//	
			//	if( obj->isKindOf( KINDOF_BRIDGE_TOWER ) )
			//	{
			//		// remove the bridge scaffold
			//		removeBridgeScaffolding( obj );
      //  
			//		// clear the repair bit from the bridge
			//		BridgeTowerBehaviorInterface *btbi = BridgeTowerBehavior::getBridgeTowerBehaviorInterfaceFromObject( obj );
			//		DEBUG_ASSERTCRASH( btbi, ("Unable to find bridge tower behavior interface") );
			//		Object *bridge = TheGameLogic->findObjectByID( btbi->getBridgeID() );
			//		DEBUG_ASSERTCRASH( bridge, ("Unable to find bridge object") );
			//		bridge->clearStatus( OBJECT_STATUS_UNDERGOING_REPAIR );
			//	
			//	}  // end if
      //
			//}  // end if

			break;

		}  // end repair

		// --------------------------------------------------------------------------------------------
		case DOZER_TASK_FORTIFY:
		{

			break;

		}  // end fortify

		// --------------------------------------------------------------------------------------------
		default:
		{

			DEBUG_CRASH(( "internalTaskCompleteOrCancelled: Unknown Dozer task '%d'", task ));
			break;

		}  // end default

	}  // end switch( task )

}  // end internalTaskCompleteOrCancelled

//-------------------------------------------------------------------------------------------------
/** If we were building something, kill the active-construction flag on it */
//-------------------------------------------------------------------------------------------------
void DozerAIUpdate::onDelete( void )
{
	Int i;

	// cancel any of the tasks we had queued up
	for( i = DOZER_TASK_FIRST; i < DOZER_NUM_TASKS; ++i )
	{
		
		if( isTaskPending( (DozerTask)i ) )
			cancelTask( (DozerTask)i );
			
	}  // end for i

	for( i = 0; i < DOZER_NUM_TASKS; i++ )
	{
		Object* goalObject = TheGameLogic->findObjectByID(m_task[i].m_targetObjectID);
		if (goalObject != NULL)
		{
			goalObject->clearModelConditionState(MODELCONDITION_ACTIVELY_BEING_CONSTRUCTED);
		}
	}
}

//-------------------------------------------------------------------------------------------------
/** Get the most recently issued task */
//-------------------------------------------------------------------------------------------------
DozerTask DozerAIUpdate::getMostRecentCommand( void )
{
	Int i;
	DozerTask mostRecentTask = DOZER_TASK_INVALID;
	UnsignedInt mostRecentFrame = 0;

	for( i = 0; i < DOZER_NUM_TASKS; i++ )
	{

		if( isTaskPending( (DozerTask)i ) )
		{

			if( m_task[ i ].m_taskOrderFrame > mostRecentFrame )
			{

				mostRecentTask = (DozerTask)i;
				mostRecentFrame = m_task[ i ].m_taskOrderFrame;

			}  // end if

		}  // end if

	}  // end for i

	return mostRecentTask;
			
}  // end getMostRecentCommand

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
const Coord3D* DozerAIUpdate::getDockPoint( DozerTask task, DozerDockPoint point )
{

	// sanity
	if( task < 0 || task >= DOZER_NUM_TASKS )
		return NULL;

	// sanity
	if( point < 0 || point >= DOZER_NUM_DOCK_POINTS )
		return NULL;

	// if the point has been set (is valid) then return it
	if( m_dockPoint[ task ][ point ].valid )
		return &m_dockPoint[ task ][ point ].location;

	// no valid point has been set for this dock point on this task
	return NULL;

}  // end getDockPoint

// ------------------------------------------------------------------------------------------------
Real DozerAIUpdate::getRepairHealthPerSecond( void ) const
{
	return getDozerAIUpdateModuleData()->m_repairHealthPercentPerSecond;
}
// ------------------------------------------------------------------------------------------------
Real DozerAIUpdate::getBoredTime( void ) const
{
	return getDozerAIUpdateModuleData()->m_boredTime;
}
// ------------------------------------------------------------------------------------------------
Real DozerAIUpdate::getBoredRange( void ) const
{
	if (getObject()->getControllingPlayer() &&
		getObject()->getControllingPlayer()->getPlayerType() == PLAYER_COMPUTER) {
		return TheAI->getAiData()->m_aiDozerBoredRadiusModifier*getDozerAIUpdateModuleData()->m_boredRange;
	}
	return getDozerAIUpdateModuleData()->m_boredRange;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
void DozerAIUpdate::aiDoCommand(const AICommandParms* parms)
{

	//
	// anytime we get a command, just remove any model condition that has us actively building
	// if we need to show that, that bit will be set anyway again during the build process
	//
	getObject()->clearModelConditionState( MODELCONDITION_ACTIVELY_CONSTRUCTING );

	if (!isAllowedToRespondToAiCommands(parms))
		return;

	// if we haven't made the dozer machine yet, do so now
	createMachines();

	switch( parms->m_cmd )
	{
		case AICMD_MOVE_AWAY_FROM_UNIT:
			{
				Object *otherObj = parms->m_obj;
				Bool otherIsDozer = false;
				if (otherObj) {
					otherIsDozer = otherObj->isKindOf(KINDOF_DOZER);
				}
				// We only want to do this if we aren't busy doing dozer things. jba.
				// Or if the other guy is a dozer too.
				if( !otherIsDozer && getCurrentTask() != DOZER_TASK_INVALID ) {
					return; // just ignore it.  jba.
				}
				// issue the command
				AIUpdateInterface::aiDoCommand(parms);
				break;
			}

		// --------------------------------------------------------------------------------------------
		case AICMD_REPAIR:
		{

			// if we have no task right now, go idle so we can immediately respond to this
			if( getCurrentTask() == DOZER_TASK_INVALID )
				aiIdle( CMD_FROM_AI );

			// do the repair
			privateRepair(parms->m_obj, parms->m_cmdSource);
			break;

		}  // end repair

		// --------------------------------------------------------------------------------------------
		case AICMD_RESUME_CONSTRUCTION:
		{

			// if we have no task right now, go idle so we can immediately respond to this
			if( getCurrentTask() == DOZER_TASK_INVALID )
				aiIdle( CMD_FROM_AI );

			// do the command
			privateResumeConstruction( parms->m_obj, parms->m_cmdSource );
			break;

		}  // end resume construction

		// --------------------------------------------------------------------------------------------
		default:
		{

			// if this is from the player, cancel our current task
			if( parms->m_cmdSource == CMD_FROM_PLAYER && getCurrentTask() != DOZER_TASK_INVALID )
				cancelTask( getCurrentTask() );

			// issue the command
			AIUpdateInterface::aiDoCommand(parms);

			// when a player issues commands, this will cause the dozer to re-evaluate what it's doing
			if( parms->m_cmdSource == CMD_FROM_PLAYER )
				m_dozerMachine->resetToDefaultState();
			break;

		}  // end default

	}  // end switch

}  // end aiDoCommand

//------------------------------------------------------------------------------------------------
void DozerAIUpdate::startBuildingSound( const AudioEventRTS *sound, ObjectID constructionSiteID )
{
	m_buildingSound = *sound;
	m_buildingSound.setObjectID( constructionSiteID );
	m_buildingSound.setPlayingHandle( TheAudio->addAudioEvent( &m_buildingSound ) );
}

//------------------------------------------------------------------------------------------------
void DozerAIUpdate::finishBuildingSound()
{
	TheAudio->removeAudioEvent( m_buildingSound.getPlayingHandle() );
}


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::crc( Xfer *xfer )
{
	// extend base class
	AIUpdateInterface::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 1;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );
 
 // extend base class
	AIUpdateInterface::xfer(xfer);

	Int numTasks = DOZER_NUM_TASKS;
	xfer->xferInt(&numTasks);
	if (numTasks != DOZER_NUM_TASKS) {
		DEBUG_CRASH(("DOZER_NUM_TASKS changed unexpectedly."));
		throw SC_INVALID_DATA;
	}
	Int i, j;
	for (i=0; i<DOZER_NUM_TASKS; i++) {
		xfer->xferObjectID(&m_task[i].m_targetObjectID);
		xfer->xferUnsignedInt(&m_task[i].m_taskOrderFrame);
	}
	xfer->xferSnapshot(m_dozerMachine);
	xfer->xferUser(&m_currentTask, sizeof(m_currentTask));

	Int dockPoints = DOZER_NUM_DOCK_POINTS;
	xfer->xferInt(&dockPoints);
	if (dockPoints!=DOZER_NUM_DOCK_POINTS) {
		DEBUG_CRASH(("DOZER_NUM_DOCK_POINTS changed unexpectedly."));
		throw SC_INVALID_DATA;
	}
	for (i=0; i<DOZER_NUM_TASKS; i++) {
		for (j=0; j<DOZER_NUM_DOCK_POINTS; j++) {
			xfer->xferBool(&m_dockPoint[i][j].valid);
			xfer->xferCoord3D(&m_dockPoint[i][j].location);
		}
	}
	xfer->xferUser(&m_buildSubTask, sizeof(m_buildSubTask));

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DozerAIUpdate::loadPostProcess( void )
{
 // extend base class
	AIUpdateInterface::loadPostProcess();
}  // end loadPostProcess


