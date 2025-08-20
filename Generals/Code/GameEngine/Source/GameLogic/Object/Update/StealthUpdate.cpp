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

// FILE: StealthUpdate.cpp ////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, May 2002
// Desc:	 An update that checks for a status bit to stealth the owning object
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_STEALTHLEVEL_NAMES
#define DEFINE_OBJECT_STATUS_NAMES

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/GameState.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Radar.h"
#include "Common/Team.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"

#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameClient/GameClient.h"

#include "GameLogic/Damage.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Weapon.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/ContainModule.h"




//-------------------------------------------------------------------------------------------------
void StealthUpdateModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "StealthDelay",									INI::parseDurationUnsignedInt,	NULL, offsetof( StealthUpdateModuleData, m_stealthDelay ) },
		{ "MoveThresholdSpeed",						INI::parseVelocityReal,					NULL, offsetof( StealthUpdateModuleData, m_stealthSpeed ) },
		{ "StealthForbiddenConditions",		INI::parseBitString32,					TheStealthLevelNames, offsetof( StealthUpdateModuleData, m_stealthLevel) },
		{ "HintDetectableConditions",	  	ObjectStatusMaskType::parseFromINI,	NULL, offsetof( StealthUpdateModuleData, m_hintDetectableStates) },
		{ "FriendlyOpacityMin",						INI::parsePercentToReal,				NULL, offsetof( StealthUpdateModuleData, m_friendlyOpacityMin ) },
		{ "FriendlyOpacityMax",						INI::parsePercentToReal,				NULL, offsetof( StealthUpdateModuleData, m_friendlyOpacityMax ) },
		{ "PulseFrequency",								INI::parseDurationUnsignedInt,	NULL, offsetof( StealthUpdateModuleData, m_pulseFrames ) },
		{ "DisguisesAsTeam",							INI::parseBool,									NULL, offsetof( StealthUpdateModuleData, m_teamDisguised ) },
		{ "RevealDistanceFromTarget",			INI::parseReal,									NULL, offsetof( StealthUpdateModuleData, m_revealDistanceFromTarget ) },
		{ "OrderIdleEnemiesToAttackMeUponReveal", INI::parseBool,					NULL, offsetof( StealthUpdateModuleData, m_orderIdleEnemiesToAttackMeUponReveal ) },
		{ "DisguiseFX",										INI::parseFXList,								NULL, offsetof( StealthUpdateModuleData, m_disguiseFX ) },
		{ "DisguiseRevealFX",							INI::parseFXList,								NULL, offsetof( StealthUpdateModuleData, m_disguiseRevealFX ) },
		{ "DisguiseTransitionTime",				INI::parseDurationUnsignedInt,  NULL, offsetof( StealthUpdateModuleData, m_disguiseTransitionFrames ) },
		{ "DisguiseRevealTransitionTime",	INI::parseDurationUnsignedInt,  NULL, offsetof( StealthUpdateModuleData, m_disguiseRevealTransitionFrames ) },
		{ "InnateStealth",								INI::parseBool,									NULL, offsetof( StealthUpdateModuleData, m_innateStealth ) },

		{ 0, 0, 0, 0 }
	};
  p.add(dataFieldParse);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpdate::StealthUpdate( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();

	m_stealthAllowedFrame = TheGameLogic->getFrame() + data->m_stealthDelay;

	//Must be enabled manually if using disguise system (bomb truck uses)
	m_enabled = !data->m_teamDisguised;

	//Added By Sadullah Nader
	//Initialization(s) inserted
	m_detectionExpiresFrame = 0;
	//
	m_pulsePhaseRate		= 0.2f;
	m_pulsePhase				= GameClientRandomValueReal(0, PI);

	m_disguiseAsPlayerIndex			= -1;
	m_disguiseAsTemplate			  = NULL;
	m_transitioningToDisguise		= false;
	m_disguised									= false;
	m_disguiseTransitionFrames	= 0;
	m_disguiseHalfpointReached  = false;

	if( data->m_innateStealth )
	{
		//Giving innate stealth units this status bit allows other code to easily check the status bit.
		getObject()->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_CAN_STEALTH ) );
	}

	// start active, since some stealths start enabled from the get-go
	setWakeFrame(getObject(), UPDATE_SLEEP_NONE);

	// we do not need to restore a disguise
	m_xferRestoreDisguise = FALSE;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpdate::~StealthUpdate( void )
{

}

//-------------------------------------------------------------------------------------------------
Bool StealthUpdate::allowedToStealth() const
{
	const Object *self = getObject();
	UnsignedInt flags = getStealthUpdateModuleData()->m_stealthLevel;

	if( flags & STEALTH_NOT_WHILE_ATTACKING && self->getStatusBits().test( OBJECT_STATUS_IS_FIRING_WEAPON ) )
	{
		//Doesn't stealth while aggressive (includes approaching).
		return FALSE;
	}

	if( flags & STEALTH_NOT_WHILE_USING_ABILITY && self->getStatusBits().test( OBJECT_STATUS_IS_USING_ABILITY ) )
	{
		//Doesn't stealth while using a special ability (starting with preparation, which takes place after unpacking).
		return FALSE;
	}

	//Do a quick preliminary test to see if we are restricted by firing particular weapons and we fired a shot last frame or this frame.
	if( flags & STEALTH_NOT_WHILE_FIRING_WEAPON && self->getStatusBits().test( OBJECT_STATUS_IS_FIRING_WEAPON ) )
	{
		if( (flags & STEALTH_NOT_WHILE_FIRING_WEAPON) == STEALTH_NOT_WHILE_FIRING_WEAPON )
		{
			//Not allowed to stealth while firing ANY weapon!
			return FALSE;
		}

		//Now do weapon specific checks.
		Weapon *weapon;
		UnsignedInt lastFrame = TheGameLogic->getFrame() - 1;

		if( flags & STEALTH_NOT_WHILE_FIRING_PRIMARY )
		{
			//Check primary weapon status
			weapon = self->getWeaponInWeaponSlot( PRIMARY_WEAPON );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return false;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_SECONDARY )
		{
			//Check secondary weapon status
			weapon = self->getWeaponInWeaponSlot( SECONDARY_WEAPON );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return false;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_TERTIARY )
		{
			//Check tertiary weapon status
			weapon = self->getWeaponInWeaponSlot( TERTIARY_WEAPON );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return false;
			}
		}
	}

	const PhysicsBehavior *physics = self->getPhysics();
	if ((flags & STEALTH_NOT_WHILE_MOVING) && physics != NULL &&
					physics->getVelocityMagnitude() > getStealthUpdateModuleData()->m_stealthSpeed)
		return false;

	if( self->testScriptStatusBit(OBJECT_STATUS_SCRIPT_UNSTEALTHED))
	{
		//We can't stealth because a script disabled this ability for this object!
		return false;
	}

	return true;
}


//---------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void StealthUpdate::hintDetectableWhileUnstealthed()
{
	Object *self = getObject();
	const StealthUpdateModuleData *md = getStealthUpdateModuleData();

	if( self && md->m_hintDetectableStates.testForAny( self->getStatusBits() ) )
	{
		if ( self->getControllingPlayer() == ThePlayerList->getLocalPlayer() )
		{
			Drawable *selfDraw = self->getDrawable();
			if ( selfDraw )
				selfDraw->setHeatVisionOpacity( 1.0f );
		}
	}
}



//-------------------------------------------------------------------------------


Real StealthUpdate::getFriendlyOpacity() const
{
	return getStealthUpdateModuleData()->m_friendlyOpacityMin;
}


//=============================================================================
// indicate how the given unit is "stealthed" with respect to a given player.
StealthLookType StealthUpdate::calcStealthedStatusForPlayer(const Object* obj, const Player* player)
{
	/*
		for stealthy things, there are these distinct "logical" states:

		-- not stealthed at all (ie, totally visible)
		-- stealthed
		-- stealthed-but-detected

		and the following visual states:

		-- normal (n)
		-- invisible (i)
		-- stealthed-but-visible-to-friendly-folks (sv)
		-- stealthed-but-visible-to-everyone-due-to-being-detected (sd)

		Let's be ubergeeks and make a matrix of the possibilities:

								Ally		Nonally
		normal:			(n)			(n)
		stealthed:	(sv)		(i)
		detected:		(sd)		(sd)


		Or, to put it another way:

		If normal, you always appear normal.
		If stealthed (and not detected), you appear as (sv) to allies and (i) to others.
		If detected, you always appears as (sd).

		Sorry, there is one more condition, stealthed, but visible to friendly folks, YET detected
		In this state we render outselves visible and we ovlerlay the detection effect as a warning
		we'll call this STEALTHLOOK_VISIBLE_FRIENDLY_DETECTED


	*/


	if (obj->isEffectivelyDead())
		return STEALTHLOOK_NONE;			// making sure he turns visible when he dies

	if( obj->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
	{
		const Team* team = obj->getTeam();
		Relationship r = team ? team->getRelationship(player->getDefaultTeam()) : NEUTRAL;
		if( !player->isPlayerActive() )
		{
			//Observer players are friends to everyone!
			r = ALLIES;
		}

// srj sez: disguised stuff doesn't work well when combined with the normal "detected" stuff.
// so special case it here.
		if (canDisguise())
		{
			if (r != ALLIES && isDisguised())
				return STEALTHLOOK_DISGUISED_ENEMY;
			else
				return STEALTHLOOK_NONE;
		}

		if( obj->getStatusBits().test( OBJECT_STATUS_DETECTED ) )			// we're detected.
		{
			if (r == ALLIES)// if we're friendly to the given player, detection DOES matter though.
				return STEALTHLOOK_VISIBLE_FRIENDLY_DETECTED;
			else
				return STEALTHLOOK_VISIBLE_DETECTED;
		}
		else
		{
			if (r == ALLIES)
			{
				// if we're friendly to the given player, detection doesn't matter.
				return STEALTHLOOK_VISIBLE_FRIENDLY;
			}
			else
			{
// srj sez: disguised stuff doesn't work well when combined with the normal "detected" stuff.
// so special case it above.
//				if( getStealthUpdateModuleData()->m_teamDisguised )
//				{
//					return STEALTHLOOK_DISGUISED_ENEMY;
//				}
				// we're effectively hidden.
				return STEALTHLOOK_INVISIBLE;
			}
		}
	}
	else
	{
		return STEALTHLOOK_NONE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime StealthUpdate::calcSleepTime() const
{
	return m_enabled ? UPDATE_SLEEP_NONE : UPDATE_SLEEP_FOREVER;
}

//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime StealthUpdate::update( void )
{

	// restore disguise if we need to from a game load
	if( m_xferRestoreDisguise == TRUE )
	{
		Drawable *draw = getObject()->getDrawable();
		Bool wasHidden = FALSE;

		// hack! if drawable was hidden (such as if we're inside a container) we must keep that state
		if( draw && draw->isDrawableEffectivelyHidden() )
			wasHidden = TRUE;

		// do the change (we get a new drawable from this)
		changeVisualDisguise();

		// restore hidden state in the new drawable
		draw = getObject()->getDrawable();
		if( wasHidden && draw )
			draw->setDrawableHidden( TRUE );

	}  // end if

	Object *self = getObject();
	UnsignedInt now = TheGameLogic->getFrame();
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();

/// @todo srj -- improve sleeping behavior. we currently just sleep when not enabled,
// and demand every-frame attention when enabled. this could probably be smartened.

	if( !m_enabled )
	{
		return calcSleepTime();
	}

	Drawable* draw = self->getDrawable();
	if (draw)
	{
		//Are we disguise transitioning (either gaining or losing disguise look?)
		/** @todo srj -- evil hack here... this whole heat-vision thing is fucked.
			don't want it on mines but no good way to do that. hack for now. */
		if (self->isKindOf(KINDOF_MINE))
		{
			// special case for mines
			draw->setEffectiveOpacity( 0.0f, 0.0f );
		}
		else if( m_disguiseTransitionFrames )
		{
			m_disguiseTransitionFrames--;
			Real factor;
			if( m_transitioningToDisguise )
			{
				factor = 1.0f - ( (Real)m_disguiseTransitionFrames / (Real)data->m_disguiseTransitionFrames );
			}
			else
			{
				factor = 1.0f - ( (Real)m_disguiseTransitionFrames / (Real)data->m_disguiseRevealTransitionFrames );
			}
			if( factor >= 0.5f && !m_disguiseHalfpointReached )
			{
				//Switch models at the halfway point
				changeVisualDisguise();

				// TheSuperHackers @fix Skyaero 06/05/2025 obtain the new drawable
				draw = getObject()->getDrawable();

				m_disguiseHalfpointReached = true;
			}
			//Opacity ranges from full to none at midpoint and full again at the end
			Real opacity = fabs( 1.0f - (factor * 2.0f) );
			Real overrideOpacity = opacity < 1.0f ? 0.0f : 1.0f;
			draw->setEffectiveOpacity( opacity, overrideOpacity );
			if( !m_disguiseTransitionFrames && !m_transitioningToDisguise )
			{
				//We're finished removing disguise so turn off stealth update.
				m_enabled = false;
				self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_STEALTHED ) );
				self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DETECTED ) );
				return calcSleepTime();
			}
		}
		else
		{
			draw->setEffectiveOpacity( 0.5f + ( Sin( m_pulsePhase ) * 0.5f ) );
			// between one half and full opacity
			m_pulsePhase += m_pulsePhaseRate;
		}
	}

/// @todo srj -- do we need to do this EVERY frame?
	Real revealDistance = getRevealDistanceFromTarget();
	if( revealDistance > 0.0f )
	{
		AIUpdateInterface *ai = self->getAI();
		if( ai )
		{
			Object *target = ai->getCurrentVictim();
			if( target )
			{
				Real distSqrd = ThePartitionManager->getDistanceSquared( self, target, FROM_CENTER_2D );
				if( distSqrd <= revealDistance * revealDistance )
				{
					//We're close enough to reveal ourselves
					markAsDetected();
					return calcSleepTime();
				}
			}
		}
	}

	// If the object is unable to Stealth, don't bother trying.
	if( !(self->getStatusBits().test(OBJECT_STATUS_CAN_STEALTH)) )
	{
		return calcSleepTime();
	}


	if (allowedToStealth())
	{
		// If I can stealth, don't attempt to Stealth until the timer is zero.
		if( m_stealthAllowedFrame > now )
		{
			return calcSleepTime();
		}

		// If we haven't stealthed yet( still destealthed ), play stealthOn here
		//if ( ( self->getStatusBits() && OBJECT_STATUS_STEALTHED ) == 0 )
		if( !self->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
		{
			AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOn();
			soundEvent.setObjectID(self->getID());
			TheAudio->addAudioEvent( &soundEvent );
		}

		// The timer is zero, so if we aren't stealthed, do so now!
		self->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_STEALTHED ) );
	}
	else
	{
		m_stealthAllowedFrame = now + getStealthUpdateModuleData()->m_stealthDelay;

		// if you are destealthing on your own free will, play sound for all to hear
		if( self->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
		{
			AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOn();
			soundEvent.setObjectID(self->getID());
			TheAudio->addAudioEvent( &soundEvent );
		}

		self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_STEALTHED ) );

		hintDetectableWhileUnstealthed();


	}


	Bool detectedStatusChangedThisFrame = FALSE;
	if (m_detectionExpiresFrame > now)
	{
		// if this is the first time being detected, play stealth off sound
		if( !self->getStatusBits().test( OBJECT_STATUS_DETECTED ) )
		{
			detectedStatusChangedThisFrame = TRUE;
			AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOff();
			soundEvent.setObjectID(self->getID());
			TheAudio->addAudioEvent( &soundEvent );
		}

		self->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DETECTED ) );
	}
	else
	{
		// if this is the first time your clearing the detected status, play the stealth on sound
		if( self->getStatusBits().test( OBJECT_STATUS_DETECTED ) )
		{
			detectedStatusChangedThisFrame = TRUE;
			//Only play sound effect if the selected object is controllable.
			if( self->isLocallyControlled() )
			{
				AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOn();
				soundEvent.setObjectID(self->getID());
				TheAudio->addAudioEvent( &soundEvent );
			}
		}

		self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DETECTED ) );
	}

	if ( detectedStatusChangedThisFrame )
	{
		//do the trick where we tell our container to recals his apparent controlling player
		//since I may have just become either detected or undetected
		if ( self->isContained() )
		{
			Object *container = self->getContainedBy();
			if ( container )
			{
				ContainModuleInterface *contain = container->getContain();
				if( contain && contain->isGarrisonable() )
				{
					contain->recalcApparentControllingPlayer();
				}
			}
		}

	}



	if (draw)
	{
		StealthLookType stealthLook = calcStealthedStatusForPlayer( self, ThePlayerList->getLocalPlayer() );
		draw->setStealthLook( stealthLook );
	}

	return calcSleepTime();
}

//-------------------------------------------------------------------------------------------------
void setWakeupIfInRange( Object *obj, void *userData)
{
	Object *victim = (Object *)userData;
	AIUpdateInterface *ai = obj->getAI();
	if (!ai) {
		return;
	}

	Real vision = obj->getVisionRange();

	Coord3D srcpos = *obj->getPosition();
	Coord3D dstpos = *victim->getPosition();

	srcpos.sub(&dstpos);
	if (srcpos.length() > vision)
		return;

	ai->wakeUpAndAttemptToTarget();

//	if( obj->isKindOf( KINDOF_SELECTABLE ) && ( obj->isAbleToAttack() || !obj->isKindOf( KINDOF_STRUCTURE ) )) {
//		Drawable *draw = obj->getDrawable();
//		if( draw ) {
//			draw->setEmoticon( "Emoticon_Alarm", 5000 );
//		}
//	}
}

//-------------------------------------------------------------------------------------------------
void StealthUpdate::markAsDetected(UnsignedInt numFrames)
{
	Object *self = getObject();
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();

	Player *thisPlayer = self->getControllingPlayer();

	//If we are disguised, remove the disguise permanently!
	if( isDisguised() )
	{
		disguiseAsObject( NULL );
	}

	UnsignedInt now = TheGameLogic->getFrame();
	if( !numFrames )
	{
		//Kris:
		//If numFrames is zero (the default value), use the stealth delay specified in the ini file.
		m_detectionExpiresFrame = now + data->m_stealthDelay;
	}
	else if ( m_detectionExpiresFrame < now + numFrames )
	{
		m_detectionExpiresFrame = now + numFrames;
	}

	if( data->m_orderIdleEnemiesToAttackMeUponReveal )
	{
		// This can't be a partitionmanager thing, because we need to know which objects can see
		// us. Therefore, walk the play list, and for each player that considers us an enemy,
		// check if any of their units can see us.

		Int numPlayers = ThePlayerList->getPlayerCount();
		for (Int n = 0; n < numPlayers; ++n)
		{

			Player *player = ThePlayerList->getNthPlayer(n);
			if (!player)
				continue;

			if (player->getRelationship(thisPlayer->getDefaultTeam()) != ENEMIES)
				continue;

			player->iterateObjects(setWakeupIfInRange, self);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void StealthUpdate::disguiseAsObject( const Object *target )
{
	Object *self = getObject();
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();
	if( target && target->getControllingPlayer() )
	{
		StealthUpdate* stealth = target->getStealth();
		if( stealth && stealth->getDisguisedTemplate() )
		{
			m_disguiseAsTemplate				= stealth->getDisguisedTemplate();
			m_disguiseAsPlayerIndex			= stealth->getDisguisedPlayerIndex();
		}
		else
		{
			m_disguiseAsTemplate				= target->getTemplate();
			m_disguiseAsPlayerIndex			= target->getControllingPlayer()->getPlayerIndex();
		}

		m_enabled										= true;
		m_transitioningToDisguise		= true; //Means we are gaining disguise over time.
		m_disguiseTransitionFrames	= data->m_disguiseTransitionFrames;
		m_disguiseHalfpointReached  = false;

		//Wake up so I can process!
		setWakeFrame( getObject(), UPDATE_SLEEP_NONE );

	}
	else if( m_disguised )
	{
		m_disguiseAsTemplate				= NULL;
		m_disguiseAsPlayerIndex			= 0;
		m_disguiseTransitionFrames	= data->m_disguiseRevealTransitionFrames;
		m_transitioningToDisguise		= false; //Means we are losing the disguise over time.
		m_disguiseHalfpointReached  = false;
	}

	Drawable *draw = self->getDrawable();
	if( draw && draw->isSelected() )
	{
		TheControlBar->markUIDirty();
	}

}

//-------------------------------------------------------------------------------------------------
void StealthUpdate::changeVisualDisguise()
{
	Object *self = getObject();
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();

	Drawable *draw = self->getDrawable();
	// We need to maintain our selection across the un/disguise, so pull selected out here.
	Bool selected = draw->isSelected();

	if( m_disguiseAsTemplate )
	{
		Player *player = ThePlayerList->getNthPlayer( m_disguiseAsPlayerIndex );

		ModelConditionFlags flags = draw->getModelConditionFlags();

		//Get rid of the old instance!
		TheGameClient->destroyDrawable( draw );

		draw = TheThingFactory->newDrawable( m_disguiseAsTemplate );
		if( draw )
		{
			TheGameLogic->bindObjectAndDrawable(self, draw);
			draw->setPosition( self->getPosition() );
			draw->setOrientation( self->getOrientation() );
			draw->setModelConditionFlags( flags );
			draw->updateDrawable();
			self->getPhysics()->resetDynamicPhysics();
			if( selected )
			{
				TheInGameUI->selectDrawable( draw );
			}
			Player *clientPlayer = ThePlayerList->getLocalPlayer();
			if( self->getControllingPlayer()->getRelationship( clientPlayer->getDefaultTeam() ) != ALLIES && clientPlayer->isPlayerActive() )
			{
				//Neutrals and enemies will see this disguised unit as the team it's disguised as.
				if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
					draw->setIndicatorColor( player->getPlayerNightColor() );
				else
					draw->setIndicatorColor( player->getPlayerColor() );
			}
			else
			{
				//If it's on our team or our ally's team, then show it's true colors.
				if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
					draw->setIndicatorColor( self->getNightIndicatorColor() );
				else
					draw->setIndicatorColor( self->getIndicatorColor() );
			}
		}

		//Play a disguise sound!
		AudioEventRTS sound = *self->getTemplate()->getPerUnitSound( "DisguiseStarted" );
		sound.setObjectID( self->getID() );
		TheAudio->addAudioEvent( &sound );

		FXList::doFXPos( data->m_disguiseFX, self->getPosition() );

		m_disguised = true;
	}
	else if( m_disguiseAsPlayerIndex != -1 )
	{
		m_disguiseAsPlayerIndex = -1;
		ModelConditionFlags flags = draw->getModelConditionFlags();

		//Get rid of the old instance!
		TheGameClient->destroyDrawable( draw );

		const ThingTemplate *tTemplate = self->getTemplate();

		// TheSuperHackers @fix helmutbuhler 13/04/2025 Fixes missing pointer assignment for the new drawable.
		// This originally caused no runtime crash because the new drawable is allocated at the same address as the previously deleted drawable via the MemoryPoolBlob.
		draw = TheThingFactory->newDrawable( tTemplate );
		if( draw )
		{
			TheGameLogic->bindObjectAndDrawable(self, draw);
			draw->setPosition( self->getPosition() );
			draw->setOrientation( self->getOrientation() );
			draw->setModelConditionFlags( flags );
			draw->updateDrawable();
			self->getPhysics()->resetDynamicPhysics();
			if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
				draw->setIndicatorColor( self->getNightIndicatorColor() );
			else
				draw->setIndicatorColor( self->getIndicatorColor() );
			if( selected )
			{
				TheInGameUI->selectDrawable( draw );
			}

			//UGH!
			//A concrete example is the bomb truck. Different payloads are displayed based on which upgrades have been
			//made. When the bomb truck disguises as something else, these subobjects are lost because the vector is
			//stored in W3DDrawModule. When we revert back to the original bomb truck, we call this function to
			//recalculate those upgraded subobjects.
			self->forceRefreshSubObjectUpgradeStatus();
		}

		Bool successfulReveal = false;
		AIUpdateInterface *ai = self->getAI();
		if( ai )
		{
			Object *currTarget = ai->getCurrentVictim();
			if( currTarget )
			{
				successfulReveal = true;
			}
		}

		//Play a reveal sound!
		AudioEventRTS sound;
		if( successfulReveal )
		{
			sound = *self->getTemplate()->getPerUnitSound( "DisguiseRevealedSuccess" );
		}
		else
		{
			sound = *self->getTemplate()->getPerUnitSound( "DisguiseRevealedFailure" );
		}
		sound.setObjectID( self->getID() );
		TheAudio->addAudioEvent( &sound );

		FXList::doFXPos( data->m_disguiseRevealFX, self->getPosition() );
		m_disguised = false;
	}

	//Reset the radar (determines color on add)
	TheRadar->removeObject( self );
	TheRadar->addObject( self );

	// couldn't possibly need to restore a disguise now :)
	m_xferRestoreDisguise = FALSE;

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void StealthUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void StealthUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// stealth allowed frame
	xfer->xferUnsignedInt( &m_stealthAllowedFrame );

	// detection expires frame
	xfer->xferUnsignedInt( &m_detectionExpiresFrame );

	// enabled
	xfer->xferBool( &m_enabled );

	// pulse phase rate
	xfer->xferReal( &m_pulsePhaseRate );

	// pulse phase
	xfer->xferReal( &m_pulsePhase );

	// disguise as player index
	xfer->xferInt( &m_disguiseAsPlayerIndex );

	// disguise as template
	AsciiString name = m_disguiseAsTemplate ? m_disguiseAsTemplate->getName() : AsciiString::TheEmptyString;
	xfer->xferAsciiString( &name );
	if( xfer->getXferMode() == XFER_LOAD )
	{

		m_disguiseAsTemplate = NULL;
		if( name.isEmpty() == FALSE )
		{

			m_disguiseAsTemplate = TheThingFactory->findTemplate( name );
			if( m_disguiseAsTemplate == NULL )
			{

				DEBUG_CRASH(( "StealthUpdate::xfer - Unknown template '%s'", name.str() ));
				throw SC_INVALID_DATA;

			}  // end if

		}  // end if

	}  // end if

	// disguise transition frames
	xfer->xferUnsignedInt( &m_disguiseTransitionFrames );

	// disguise halfpoint reached
	xfer->xferBool( &m_disguiseHalfpointReached );

	// transitioning to disguise
	xfer->xferBool( &m_transitioningToDisguise );

	// disguised
	xfer->xferBool( &m_disguised );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void StealthUpdate::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

	//
	// we will need to restore our disguise when the game is ready to run ... NOTE that we
	// cannot restore it here because if we called changeVisualDisguise() it would
	// destroy our drawable and create new stuff.  The destruction of a drawable during
	// a load is *very* bad ... it has a snapshot instance in the game state, other things
	// may be pointing at it etc.
	//
	if( isDisguised() )
		m_xferRestoreDisguise = TRUE;

}  // end loadPostProcess
