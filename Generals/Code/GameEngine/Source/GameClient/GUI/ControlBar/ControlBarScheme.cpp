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

// FILE: ControlBarScheme.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//                                                                          
//                       Electronic Arts Pacific.                          
//                                                                          
//                       Confidential Information                           
//                Copyright (C) 2002 - All Rights Reserved                  
//                                                                          
//-----------------------------------------------------------------------------
//
//	created:	Apr 2002
//
//	Filename: 	ControlBarScheme.cpp
//
//	author:		Chris Huybregts
//	
//	purpose:	Contains all the Command bar goodness in terms of how it looks
//						For instrucitons on how to use, please see it's .h file
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Player.h"
#include "Common/PlayerTemplate.h"
#include "Common/Recorder.h"
#include "GameClient/ControlBarScheme.h"
#include "GameClient/Display.h"
#include "GameClient/ControlBar.h"
#include "GameClient/Image.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GadgetPushButton.h"
#ifdef RTS_INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

//-----------------------------------------------------------------------------
// DEFINES ////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
enum{
	COMMAND_BAR_SIZE_OFFSET = 0
};

const FieldParse ControlBarSchemeManager::m_controlBarSchemeFieldParseTable[] = 
{

	{ "ImagePart",						ControlBarSchemeManager::parseImagePart,			NULL, NULL },
	{ "AnimatingPart",				ControlBarSchemeManager::parseAnimatingPart,	NULL, NULL },
	{ "ScreenCreationRes",		INI::parseICoord2D,						NULL, offsetof( ControlBarScheme, m_ScreenCreationRes ) },
	{ "Side",									INI::parseAsciiString,				NULL, offsetof( ControlBarScheme, m_side ) },
	{ "QueueButtonImage",			INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_buttonQueueImage ) },
	{ "RightHUDImage",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_rightHUDImage ) },
	{ "BuildUpClockColor",		INI::parseColorInt,						NULL, offsetof( ControlBarScheme, m_buildUpClockColor ) },
	{ "ButtonBorderBuildColor",			INI::parseColorInt,						NULL, offsetof( ControlBarScheme, m_borderBuildColor ) },
	{ "CommandBarBorderColor",			INI::parseColorInt,						NULL, offsetof( ControlBarScheme, m_commandBarBorderColor ) },
	{ "ButtonBorderActionColor",		INI::parseColorInt,						NULL, offsetof( ControlBarScheme, m_borderActionColor ) },
	{ "ButtonBorderUpgradeColor",		INI::parseColorInt,						NULL, offsetof( ControlBarScheme, m_borderUpgradeColor ) },
	{ "ButtonBorderSystemColor",		INI::parseColorInt,						NULL, offsetof( ControlBarScheme, m_borderSystemColor ) },
	{ "OptionsButtonEnable",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_optionsButtonEnable ) },
	{ "OptionsButtonHightlited",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_optionsButtonHightlited ) },
	{ "OptionsButtonPushed",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_optionsButtonPushed ) },
	{ "OptionsButtonDisabled",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_optionsButtonDisabled ) },
	{ "IdleWorkerButtonEnable",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_idleWorkerButtonEnable ) },
	{ "IdleWorkerButtonHightlited",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_idleWorkerButtonHightlited ) },
	{ "IdleWorkerButtonPushed",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_idleWorkerButtonPushed ) },
	{ "IdleWorkerButtonDisabled",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_idleWorkerButtonDisabled ) },
	{ "BuddyButtonEnable",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_buddyButtonEnable ) },
	{ "BuddyButtonHightlited",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_buddyButtonHightlited ) },
	{ "BuddyButtonPushed",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_buddyButtonPushed ) },
	{ "BuddyButtonDisabled",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_buddyButtonDisabled) },
	{ "BeaconButtonEnable",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_beaconButtonEnable ) },
	{ "BeaconButtonHightlited",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_beaconButtonHightlited ) },
	{ "BeaconButtonPushed",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_beaconButtonPushed ) },
	{ "BeaconButtonDisabled",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_beaconButtonDisabled ) },
	{ "GenBarButtonIn",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_genBarButtonIn ) },
	{ "GenBarButtonOn",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_genBarButtonOn ) },
	{ "ToggleButtonUpIn",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_toggleButtonUpIn ) },
	{ "ToggleButtonUpOn",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_toggleButtonUpOn ) },
	{ "ToggleButtonUpPushed",		INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_toggleButtonUpPushed ) },
	{ "ToggleButtonDownIn",			INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_toggleButtonDownIn ) },
	{ "ToggleButtonDownOn", 		INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_toggleButtonDownOn ) },
	{ "ToggleButtonDownPushed",	INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_toggleButtonDownPushed ) },

	{ "GeneralButtonEnable",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_generalButtonEnable ) },
	{ "GeneralButtonHightlited",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_generalButtonHightlited ) },
	{ "GeneralButtonPushed",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_generalButtonPushed ) },
	{ "GeneralButtonDisabled",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_generalButtonDisabled ) },

	{ "UAttackButtonEnable",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_uAttackButtonEnable ) },
	{ "UAttackButtonHightlited",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_uAttackButtonHightlited ) },
	{ "UAttackButtonPushed",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_uAttackButtonPushed ) },

	{ "GenArrow",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_genArrow) },

	{ "MinMaxButtonEnable",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_minMaxButtonEnable ) },
	{ "MinMaxButtonHightlited",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_minMaxButtonHightlited ) },
	{ "MinMaxButtonPushed",				INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_minMaxButtonPushed ) },

	{ "MinMaxUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_minMaxUL ) },
	{ "MinMaxLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_minMaxLR ) },

	{ "GeneralUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_generalUL ) },
	{ "GeneralLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_generalLR ) },

	{ "UAttackUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_uAttackUL ) },
	{ "UAttackLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_uAttackLR ) },

	{ "OptionsUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_optionsUL ) },
	{ "OptionsLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_optionsLR ) },

	{ "WorkerUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_workerUL ) },
	{ "WorkerLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_workerLR ) },

	{ "ChatUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_chatUL ) },
	{ "ChatLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_chatLR ) },

	{ "BeaconUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_beaconUL ) },
	{ "BeaconLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_beaconLR ) },

	{ "PowerBarUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_powerBarUL ) },
	{ "PowerBarLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_powerBarLR ) },

	{ "MoneyUL", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_moneyUL ) },
	{ "MoneyLR", 		INI::parseICoord2D,				NULL, offsetof( ControlBarScheme, m_moneyLR ) },

	{ "CommandMarkerImage",		INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_commandMarkerImage) },
	{ "ExpBarForegroundImage",		INI::parseMappedImage,				NULL, offsetof( ControlBarScheme, m_expBarForeground) },

	{ NULL,										NULL,													NULL, 0 }  // keep this last

};

// used to parse the anim types that each animation part of a scheme can have
static const LookupListRec AnimTypeNames[] = 
{
	{ "SLIDE_RIGHT", ControlBarSchemeAnimation::CB_ANIM_SLIDE_RIGHT },
	{ NULL, 0	}
};

static void animSlideRight( ControlBarSchemeAnimation *anim );

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
ControlBarSchemeImage::ControlBarSchemeImage( void )
{
	m_name.clear();
	m_position.x = m_position.y = 0;
	m_size.x = m_size.y = 0;
	m_image = NULL;
	m_layer = 0;
}

ControlBarSchemeImage::~ControlBarSchemeImage( void )
{
	m_image = NULL;
}

ControlBarSchemeAnimation::ControlBarSchemeAnimation( void )
{
	// Added by Sadullah Nader
	// Initializations missing and needed
	
	m_animDuration = 0;
	m_finalPos.x = m_finalPos.y = 0;

	// End Add
	m_name.clear();
	m_animType = 0;
	m_animImage = NULL;
	m_startPos.x = m_startPos.y = 0;
	m_currentFrame = 0;
}

ControlBarSchemeAnimation::~ControlBarSchemeAnimation( void )
{
	m_animImage = NULL;
}


void ControlBarScheme::reset(void)
{
	for (Int i = 0; i < MAX_CONTROL_BAR_SCHEME_IMAGE_LAYERS; i++)
	{
		ControlBarSchemeImageList::iterator it = m_layer[i].begin();

		while (it != m_layer[i].end())
		{
			ControlBarSchemeImage *im = *it;
			if( im )
				delete im;
			im = NULL;
			it ++;	
		}
		m_layer[i].clear();
	
	}


	ControlBarSchemeAnimationList::iterator it = m_animations.begin();

	while (it != m_animations.end())
	{
		ControlBarSchemeAnimation *anim = *it;
		if( anim )
		{
			anim->m_animImage = NULL;
			delete(anim);
		}
		anim = NULL;
		it ++;	
	}
	m_animations.clear();

	m_name.clear();
	m_ScreenCreationRes.x = m_ScreenCreationRes.y = 0;
	m_side.clear();
	m_buttonQueueImage = NULL;
	m_rightHUDImage = NULL;
		m_optionsButtonEnable = NULL;
	m_optionsButtonHightlited = NULL;
	m_optionsButtonPushed = NULL;
	m_optionsButtonDisabled = NULL;

	m_idleWorkerButtonEnable = NULL;
	m_idleWorkerButtonHightlited = NULL;
	m_idleWorkerButtonPushed = NULL;
	m_idleWorkerButtonDisabled = NULL;

	m_buddyButtonEnable = NULL;
	m_buddyButtonHightlited = NULL;
	m_buddyButtonPushed = NULL;
	m_buddyButtonDisabled = NULL;

	m_beaconButtonEnable = NULL;
	m_beaconButtonHightlited = NULL;
	m_beaconButtonPushed = NULL;
	m_beaconButtonDisabled = NULL;

	m_genBarButtonIn = NULL;
	m_genBarButtonOn = NULL;

	m_toggleButtonUpIn = NULL;
	m_toggleButtonUpOn = NULL;
	m_toggleButtonUpPushed = NULL;
	m_toggleButtonDownIn = NULL;
	m_toggleButtonDownOn = NULL;
	m_toggleButtonDownPushed = NULL;

	m_commandMarkerImage = NULL;
	m_expBarForeground = NULL;
		
}

ControlBarScheme::~ControlBarScheme(void)
{
	reset();
}

ControlBarScheme::ControlBarScheme(void)
{

	m_animations.clear();
	for (Int i = 0; i < MAX_CONTROL_BAR_SCHEME_IMAGE_LAYERS; i++)
		m_layer[i].clear();
	m_name.clear();
	m_ScreenCreationRes.x = m_ScreenCreationRes.y = 0;
	m_side.clear();
	m_buttonQueueImage = NULL;
	m_rightHUDImage = NULL;
	m_buildUpClockColor = GameMakeColor(0,0,0,100);
	m_borderBuildColor = GAME_COLOR_UNDEFINED;
	m_borderActionColor = GAME_COLOR_UNDEFINED;
	m_borderUpgradeColor = GAME_COLOR_UNDEFINED;
	m_borderSystemColor = GAME_COLOR_UNDEFINED;
	//m_buttonQueueImage = TheMappedImageCollection("")
	m_optionsButtonEnable = NULL;
	m_optionsButtonHightlited = NULL;
	m_optionsButtonPushed = NULL;
	m_optionsButtonDisabled = NULL;
	
	// Added By Sadullah Nader
	// Initializations needed 
	
	m_commandBarBorderColor = 0;
	
	//
	m_idleWorkerButtonEnable = NULL;
	m_idleWorkerButtonHightlited = NULL;
	m_idleWorkerButtonPushed = NULL;
	m_idleWorkerButtonDisabled = NULL;

	m_buddyButtonEnable = NULL;
	m_buddyButtonHightlited = NULL;
	m_buddyButtonPushed = NULL;
	m_buddyButtonDisabled= NULL;

	m_beaconButtonEnable = NULL;
	m_beaconButtonHightlited = NULL;
	m_beaconButtonPushed = NULL;
	m_beaconButtonDisabled= NULL;

	m_genBarButtonIn = NULL;
	m_genBarButtonOn = NULL;

	m_toggleButtonUpIn = NULL;
	m_toggleButtonUpOn = NULL;
	m_toggleButtonUpPushed = NULL;
	m_toggleButtonDownIn = NULL;
	m_toggleButtonDownOn = NULL;
	m_toggleButtonDownPushed = NULL;

	m_commandMarkerImage = NULL;
	m_expBarForeground = NULL;

	m_generalButtonEnable = NULL;
	m_generalButtonHightlited = NULL;
	m_generalButtonPushed = NULL;
	m_generalButtonDisabled = NULL;

	m_uAttackButtonEnable = NULL;
	m_uAttackButtonHightlited = NULL;
	m_uAttackButtonPushed = NULL;
	
	m_genArrow = NULL;

	m_minMaxButtonEnable = NULL;
	m_minMaxButtonHightlited = NULL;
	m_minMaxButtonPushed = NULL;

	
	m_minMaxUL.x = 0;
	m_minMaxLR.x = 0;

	m_generalUL.x = 0;
	m_generalLR.x = 0;

	m_uAttackUL.x = 0;
	m_uAttackLR.x = 0;

	m_optionsUL.x = 0;
	m_optionsLR.x = 0;

	m_workerUL.x = 0;
	m_workerLR.x = 0;

	m_chatUL.x = 0;
	m_chatLR.x = 0;

	m_beaconUL.x = 0;
	m_beaconLR.x = 0;

	m_powerBarUL.x = 0;
	m_powerBarLR.x = 0;

	m_minMaxUL.y = 0;
	m_minMaxLR.y = 0;

	m_generalUL.y = 0;
	m_generalLR.y = 0;

	m_uAttackUL.y = 0;
	m_uAttackLR.y = 0;

	m_optionsUL.y = 0;
	m_optionsLR.y = 0;

	m_workerUL.y = 0;
	m_workerLR.y = 0;

	m_chatUL.y = 0;
	m_chatLR.y = 0;

	m_beaconUL.y = 0;
	m_beaconLR.y = 0;

	m_powerBarUL.y = 0;
	m_powerBarLR.y = 0;
	
	m_moneyUL.x = 0;
	m_moneyUL.y = 0;
	
	m_moneyLR.x = 0;
	m_moneyLR.y = 0;
}


void ControlBarScheme::init(void)
{
	if(TheControlBar)
	{
		TheControlBar->switchControlBarStage(CONTROL_BAR_STAGE_DEFAULT);
		TheControlBar->updateBuildQueueDisabledImages( m_buttonQueueImage );
		TheControlBar->updateRightHUDImage(m_rightHUDImage);
		TheControlBar->updateBuildUpClockColor( m_buildUpClockColor );
		TheControlBar->updateCommanBarBorderColors(m_borderBuildColor, m_borderActionColor, m_borderUpgradeColor,m_borderSystemColor);
		TheControlBar->updateBorderColor(m_commandBarBorderColor);
		//TheControlBar->updateCommandMarkerImage(m_commandMarkerImage);
		TheControlBar->updateSlotExitImage(m_commandMarkerImage);
		TheControlBar->updateUpDownImages(m_toggleButtonUpIn, m_toggleButtonUpOn, m_toggleButtonUpPushed, m_toggleButtonDownIn, m_toggleButtonDownOn, m_toggleButtonDownPushed, m_generalButtonEnable, m_generalButtonHightlited);
		TheControlBar->setArrowImage( m_genArrow);
	}
	GameWindow *win = NULL;
	Coord2D resMultiplier;
	resMultiplier.x = TheDisplay->getWidth()/INT_TO_REAL(m_ScreenCreationRes.x) ;
	resMultiplier.y = TheDisplay->getHeight()/INT_TO_REAL(m_ScreenCreationRes.y);

	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:PopupCommunicator" ) );
	if(win)	
	{
//		DEBUG_ASSERTCRASH(m_buddyButtonEnable,     ("No enable button image for communicator in scheme %s!\n", m_name.str()));
//		DEBUG_ASSERTCRASH(m_buddyButtonHightlited, ("No hilite button image for communicator in scheme %s!\n", m_name.str()));
//		DEBUG_ASSERTCRASH(m_buddyButtonPushed,     ("No pushed button image for communicator in scheme %s!\n", m_name.str()));
		GadgetButtonSetEnabledImage(win, m_buddyButtonEnable);
		GadgetButtonSetHiliteImage(win, m_buddyButtonHightlited);
		GadgetButtonSetHiliteSelectedImage(win, m_buddyButtonPushed);
		GadgetButtonSetDisabledImage(win, m_buddyButtonDisabled);
		
		Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_chatUL.x * resMultiplier.x - parX;
			y = m_chatUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_chatUL.x * resMultiplier.x;
			y = m_chatUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_chatLR.x - m_chatUL.x)*resMultiplier.x + COMMAND_BAR_SIZE_OFFSET,(m_chatLR.y - m_chatUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);
	}
	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:ButtonIdleWorker" ) );
	if(win)	
	{
		GadgetButtonSetEnabledImage(win, m_idleWorkerButtonEnable);
		GadgetButtonSetHiliteImage(win, m_idleWorkerButtonHightlited);
		GadgetButtonSetHiliteSelectedImage(win, m_idleWorkerButtonPushed);
		GadgetButtonSetDisabledImage(win, m_idleWorkerButtonDisabled);
		
		Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_workerUL.x * resMultiplier.x - parX;
			y = m_workerUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_workerUL.x * resMultiplier.x;
			y = m_workerUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		
		win->winSetSize((m_workerLR.x - m_workerUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_workerLR.y - m_workerUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);

	}
	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:ExpBarForeground" ) );
	if(win)	
	{
		win->winSetEnabledImage(0, m_expBarForeground);
	}
	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:ButtonOptions" ) );
	if(win)	
	{
		GadgetButtonSetEnabledImage(win, m_optionsButtonEnable);
		GadgetButtonSetHiliteImage(win, m_optionsButtonHightlited);
		GadgetButtonSetHiliteSelectedImage(win, m_optionsButtonPushed);
		GadgetButtonSetDisabledImage(win, m_optionsButtonDisabled);
		Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_optionsUL.x * resMultiplier.x - parX;
			y = m_optionsUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_optionsUL.x * resMultiplier.x;
			y = m_optionsUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_optionsLR.x - m_optionsUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_optionsLR.y - m_optionsUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);
	}
	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:ButtonPlaceBeacon" ) );
	if(win)	
	{
		GadgetButtonSetEnabledImage(win, m_beaconButtonEnable);
		GadgetButtonSetHiliteImage(win, m_beaconButtonHightlited);
		GadgetButtonSetHiliteSelectedImage(win, m_beaconButtonPushed);
		GadgetButtonSetDisabledImage(win, m_beaconButtonDisabled);

		Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_beaconUL.x * resMultiplier.x - parX;
			y = m_beaconUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_beaconUL.x * resMultiplier.x;
			y = m_beaconUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_beaconLR.x - m_beaconUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_beaconLR.y - m_beaconUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);
	}
	
	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:MoneyDisplay" ) );
	if(win)	
	{

		Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_moneyUL.x * resMultiplier.x - parX;
			y = m_moneyUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_moneyUL.x * resMultiplier.x;
			y = m_moneyUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_moneyLR.x - m_moneyUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_moneyLR.y - m_moneyUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);
	}

	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:PowerWindow" ) );
	if(win)	
	{

		Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_powerBarUL.x * resMultiplier.x - parX;
			y = m_powerBarUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_powerBarUL.x * resMultiplier.x;
			y = m_powerBarUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_powerBarLR.x - m_powerBarUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_powerBarLR.y - m_powerBarUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);
		DEBUG_LOG(("Power Bar UL X:%d Y:%d LR X:%d Y:%d size X:%d Y:%d\n",m_powerBarUL.x, m_powerBarUL.y,m_powerBarLR.x, m_powerBarLR.y, (m_powerBarLR.x - m_powerBarUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_powerBarLR.y - m_powerBarUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET  ));
	}	

	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:ButtonGeneral" ) );
	if(win)	
	{

		GadgetButtonSetEnabledImage(win, m_generalButtonEnable);
		GadgetButtonSetHiliteImage(win, m_generalButtonHightlited);
		GadgetButtonSetHiliteSelectedImage(win, m_generalButtonPushed);
		GadgetButtonSetDisabledImage(win, m_generalButtonDisabled);

				Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_generalUL.x * resMultiplier.x - parX;
			y = m_generalUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_generalUL.x * resMultiplier.x;
			y = m_generalUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_generalLR.x - m_generalUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_generalLR.y - m_generalUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);
	}
	
	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:ButtonLarge" ) );
	if(win)	
	{
		// The images are set above
//		GadgetButtonSetEnabledImage(win, m_minMaxButtonEnable);
//		GadgetButtonSetHiliteImage(win, m_minMaxButtonHightlited);
//		GadgetButtonSetHiliteSelectedImage(win, m_minMaxButtonPushed);
	
				Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_minMaxUL.x * resMultiplier.x - parX;
			y = m_minMaxUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_minMaxUL.x * resMultiplier.x;
			y = m_minMaxUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_minMaxLR.x - m_minMaxUL.x)*resMultiplier.x + COMMAND_BAR_SIZE_OFFSET,(m_minMaxLR.y - m_minMaxUL.y)*resMultiplier.y + COMMAND_BAR_SIZE_OFFSET);
	}
	
	win= TheWindowManager->winGetWindowFromId( NULL, TheNameKeyGenerator->nameToKey( "ControlBar.wnd:WinUAttack" ) );
	if(win)	
	{
		win->winSetEnabledImage(0,m_uAttackButtonEnable);
		win->winSetDisabledImage(0,m_uAttackButtonHightlited);
		
		Int x, y;
		GameWindow* parent =win->winGetParent();
		if(parent)
		{
			Int parX, parY;
			parent->winGetScreenPosition(&parX, &parY);
			x = m_uAttackUL.x * resMultiplier.x - parX;
			y = m_uAttackUL.y * resMultiplier.y - parY;
		}
		else
		{
			x = m_uAttackUL.x * resMultiplier.x;
			y = m_uAttackUL.y * resMultiplier.y;
		}
		win->winSetPosition(x,y );
		win->winSetSize((m_uAttackLR.x - m_uAttackUL.x)*resMultiplier.x+ COMMAND_BAR_SIZE_OFFSET,(m_uAttackLR.y - m_uAttackUL.y)*resMultiplier.y+ COMMAND_BAR_SIZE_OFFSET);
	}
}

//
// Add an animation to the animation list
//-----------------------------------------------------------------------------
void ControlBarScheme::addAnimation( ControlBarSchemeAnimation *schemeAnim )
{
	if( !schemeAnim )
	{
		DEBUG_ASSERTCRASH(FALSE,("Trying to add a null animation to the controlbarscheme\n"));
		return;
	}
	m_animations.push_back( schemeAnim );
}// addAnimation

//
// Add an image to the proper layer list
//-----------------------------------------------------------------------------
void ControlBarScheme::addImage( ControlBarSchemeImage *schemeImage )
{
	if( !schemeImage )
	{
		DEBUG_ASSERTCRASH(FALSE,("Trying to add a null image to the controlbarscheme\n"));
		return;
	}

	if(schemeImage->m_layer < 0 || schemeImage->m_layer >= MAX_CONTROL_BAR_SCHEME_IMAGE_LAYERS)
	{
		DEBUG_ASSERTCRASH(FALSE,("SchemeImage %s attempted to be added to layer %d which is not Between to %d, %d\n",
								 schemeImage->m_name.str(), schemeImage->m_layer, 0, MAX_CONTROL_BAR_SCHEME_IMAGE_LAYERS));
		// bring the foobar to the front so we make it obvious that something's wrong
		schemeImage->m_layer = 0;
	}

	m_layer[schemeImage->m_layer].push_back(schemeImage);
}// addImage

//
// Update the position of the image that's animating
//-----------------------------------------------------------------------------
void ControlBarScheme::updateAnim (ControlBarSchemeAnimation * anim)
{
	switch(anim->m_animType)
	{
		case ControlBarSchemeAnimation::CB_ANIM_SLIDE_RIGHT:
		{
			animSlideRight( anim );
			break;
		}
		default:
		{
			DEBUG_ASSERTCRASH(FALSE,("We tried to animate but not animate function was found %d", anim->m_animType));
		}
	}
}

//
// Add an image to the proper layer list
//-----------------------------------------------------------------------------
void ControlBarScheme::update( void )
{
	ControlBarSchemeAnimationList::iterator it = m_animations.begin();
	
	while (it != m_animations.end())
	{
		ControlBarSchemeAnimation *anim = *it;
		if( !anim )
		{
			DEBUG_ASSERTCRASH(FALSE,("THere's no Animation in the ControlBarSchemeAnimationList:m_animations"));
			return;
		}
		updateAnim( anim );
		it ++;	
	}
	
}

//
// Loop through the propper lists and draw everything for the forground
//-----------------------------------------------------------------------------
void ControlBarScheme::drawForeground( Coord2D multi, ICoord2D offset )
{
	for(Int i = CONTROL_BAR_SCHEME_FOREGROUND_IMAGE_LAYERS - 1; i >= 0; i--)
	{
		ControlBarSchemeImageList::iterator it = m_layer[i].begin();
		while (it != m_layer[i].end())
		{	
			ControlBarSchemeImage *schemeImage = *it;
			if( !schemeImage )
			{
				DEBUG_ASSERTCRASH(FALSE,("There is no ControlBarSchemeImage found in the m_layer list"));
				it++;
				continue;
			}
			
			// if we dont' have an image, don't try to draw it
			if(!schemeImage->m_image)
			{
				it++;
				continue;
			}
			
			// draw the image
			TheDisplay->drawImage(schemeImage->m_image, schemeImage->m_position.x * multi.x + offset.x,
														schemeImage->m_position.y * multi.y + offset.y, 
														(schemeImage->m_position.x + schemeImage->m_size.x) * multi.x + offset.x,
														(schemeImage->m_position.y + schemeImage->m_size.y) * multi.y + offset.y);
			
			it ++;	
		}
	}
}

//
// Add an image to the proper layer list
//-----------------------------------------------------------------------------
void ControlBarScheme::drawBackground( Coord2D multi, ICoord2D offset )
{
	
	for(Int i = MAX_CONTROL_BAR_SCHEME_IMAGE_LAYERS - 1; i >= CONTROL_BAR_SCHEME_FOREGROUND_IMAGE_LAYERS; i--)
	{
		ControlBarSchemeImageList::iterator it = m_layer[i].begin();
		while (it != m_layer[i].end())
		{	
			ControlBarSchemeImage *schemeImage = *it;
			if( !schemeImage )
			{
				DEBUG_ASSERTCRASH(FALSE,("There is no ControlBarSchemeImage found in the m_layer list"));
				it++;
				continue;
			}
			
			// if we don't have an image, don't try to draw it
			if(!schemeImage->m_image)
			{
				it++;
				continue;
			}
			
			// draw it
			TheDisplay->drawImage(schemeImage->m_image, schemeImage->m_position.x * multi.x + offset.x,
														schemeImage->m_position.y * multi.y + offset.y, 
														(schemeImage->m_position.x + schemeImage->m_size.x) * multi.x + offset.x,
														(schemeImage->m_position.y + schemeImage->m_size.y) * multi.y + offset.y);
			
			it ++;	
		}
	}
}


//
// Constructor for the manager
//-----------------------------------------------------------------------------
ControlBarSchemeManager::ControlBarSchemeManager( void )
{
	m_currentScheme = NULL;
	m_schemeList.clear();
	m_multiplyer.x = m_multiplyer.y = 1;
}

//
// Destructor for the Manager
//-----------------------------------------------------------------------------
ControlBarSchemeManager::~ControlBarSchemeManager( void )
{
	ControlBarSchemeList::iterator it = m_schemeList.begin();

	// iterate through the list and return the scheme that we're looking for
	while (it != m_schemeList.end())
	{
		ControlBarScheme *CBScheme = *it;
		if( CBScheme )
			delete CBScheme;
		CBScheme = NULL;
		it ++;	
	}
	m_schemeList.clear();
	m_currentScheme = NULL;

}

//
// Parse the Image Part of the command bar
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::parseImagePart(INI *ini, void *instance, void* /*store*/, const void* /*userData*/)
{
	static const FieldParse myFieldParse[] = 
		{
			{ "Position",				INI::parseICoord2D,				NULL, offsetof( ControlBarSchemeImage, m_position ) },
			{ "Size",						INI::parseICoord2D,				NULL, offsetof( ControlBarSchemeImage, m_size ) },
      { "ImageName",			INI::parseMappedImage,		NULL, offsetof( ControlBarSchemeImage, m_image ) },
			{ "Layer",					INI::parseInt,						NULL, offsetof( ControlBarSchemeImage, m_layer ) },
			{ NULL,							NULL,											NULL, 0 }  // keep this last
		};

	ControlBarSchemeImage *schemeImage = NEW ControlBarSchemeImage;
	ini->initFromINI(schemeImage, myFieldParse);
	((ControlBarScheme*)instance)->addImage(schemeImage);

}

//
// each animation part contains and image, parse it
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::parseAnimatingPartImage(INI *ini, void *instance, void* /*store*/, const void* /*userData*/)
{
	static const FieldParse myFieldParse[] = 
		{
			{ "Position",				INI::parseICoord2D,				NULL, offsetof( ControlBarSchemeImage, m_position ) },
			{ "Size",						INI::parseICoord2D,				NULL, offsetof( ControlBarSchemeImage, m_size ) },
      { "ImageName",			INI::parseMappedImage,		NULL, offsetof( ControlBarSchemeImage, m_image ) },
			{ "Layer",					INI::parseInt,						NULL, offsetof( ControlBarSchemeImage, m_layer ) },
			{ NULL,							NULL,											NULL, 0 }  // keep this last
		};

	ControlBarSchemeImage *schemeImage = NEW ControlBarSchemeImage;
	ini->initFromINI(schemeImage, myFieldParse);
	((ControlBarSchemeAnimation*)instance)->m_animImage = schemeImage;

}

//
// parse the animating part of the control bar scheme
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::parseAnimatingPart(INI *ini, void *instance, void* /*store*/, const void* /*userData*/)
{
	static const FieldParse myFieldParse[] = 
		{
			{ "Name",						INI::parseAsciiString,		NULL, offsetof( ControlBarSchemeAnimation, m_name ) },
      { "Animation",			INI::parseLookupList,			AnimTypeNames, offsetof( ControlBarSchemeAnimation, m_animType ) },
			{ "Duration",				INI::parseDurationUnsignedInt,			NULL, offsetof( ControlBarSchemeAnimation, m_animDuration ) },
			{ "FinalPos",				INI::parseICoord2D,			NULL, offsetof( ControlBarSchemeAnimation, m_finalPos ) },
			{ "ImagePart",			ControlBarSchemeManager::parseAnimatingPartImage,	NULL, NULL },
			{ NULL,							NULL,											NULL, 0 }  // keep this last
		};
	
	ControlBarSchemeAnimation *schemeAnim = NEW ControlBarSchemeAnimation;
	ini->initFromINI(schemeAnim, myFieldParse);
	((ControlBarScheme*)instance)->addAnimation(schemeAnim);
	((ControlBarScheme*)instance)->addImage(schemeAnim->m_animImage);
}


//
// Create a new control bar and return it.  Link it into our control bar list
//-----------------------------------------------------------------------------
ControlBarScheme *ControlBarSchemeManager::newControlBarScheme( AsciiString name )
{
	ControlBarScheme *cbScheme = 	findControlBarScheme(name);
	if(cbScheme)
	{
		DEBUG_ASSERTCRASH(false,("We're overwriting a previous control bar scheme %s",name.str()));
		cbScheme->reset();
		cbScheme->m_name.set( name );
		cbScheme->m_name.toLower();
		return cbScheme;		
	}

	cbScheme = NEW ControlBarScheme;

	if( !cbScheme  || name.isEmpty() )
	{
		DEBUG_ASSERTCRASH(FALSE,("Could not create controlbar %s", name.str()));
		return NULL;
	}

	cbScheme->m_name.set( name );
	cbScheme->m_name.toLower();

	m_schemeList.push_back(cbScheme);
		
	return cbScheme;
}

//
// Find a scheme based on name
//-----------------------------------------------------------------------------
ControlBarScheme *ControlBarSchemeManager::findControlBarScheme( AsciiString name )
{
	name.toLower();
	
	ControlBarSchemeList::iterator it = m_schemeList.begin();

	// iterate through the list and return the scheme that we're looking for
	while (it != m_schemeList.end())
	{
		ControlBarScheme *CBScheme = *it;
		if( !CBScheme )
		{
			DEBUG_ASSERTCRASH(FALSE,("There's no ControlBarScheme in the ControlBarSchemeList:m_schemeList"));
			return NULL;
		}
		if(CBScheme->m_name.compareNoCase( name ) == 0)
			return CBScheme;
		it ++;	
	}
	return NULL;
}

//
// Preload assets
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::preloadAssets( TimeOfDay timeOfDay )
{
	for (ControlBarSchemeList::iterator it = m_schemeList.begin(); it != m_schemeList.end(); ++it)
	{
		ControlBarScheme *CBScheme = *it;
		if( !CBScheme )
		{
			DEBUG_ASSERTCRASH(FALSE,("There's no ControlBarScheme in the ControlBarSchemeList:m_schemeList"));
			continue;
		}

		if (CBScheme->m_buttonQueueImage)
		{
			TheDisplay->preloadTextureAssets(CBScheme->m_buttonQueueImage->getFilename());
		}

		if (CBScheme->m_rightHUDImage)
		{
			TheDisplay->preloadTextureAssets(CBScheme->m_rightHUDImage->getFilename());
		}

		for (Int layer = 0; layer < MAX_CONTROL_BAR_SCHEME_IMAGE_LAYERS; ++layer)
		{
			for (ControlBarScheme::ControlBarSchemeImageList::iterator listIt = CBScheme->m_layer[layer].begin(); listIt != CBScheme->m_layer[layer].end(); ++listIt)
			{
				ControlBarSchemeImage *cbImage = *listIt;
				if (cbImage)
				{
					const Image *image = TheMappedImageCollection->findImageByName( cbImage->m_name );
					if (image)
					{
						TheDisplay->preloadTextureAssets(image->getFilename());
					}
				}
			}
		}
	}
}

//
// Find a scheme based on name
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::init( void )
{

	INI ini;
	// Read from INI all the ControlBarSchemes
	ini.load( AsciiString( "Data\\INI\\Default\\ControlBarScheme.ini" ), INI_LOAD_OVERWRITE, NULL );
	ini.load( AsciiString( "Data\\INI\\ControlBarScheme.ini" ), INI_LOAD_OVERWRITE, NULL );

//	//Load the user modified control bar schemes
//	WIN32_FIND_DATA findData;
//	AsciiString userDataPath;
//	if(TheGlobalData)
//	{
//		userDataPath.format("%sINI\\ControlBarScheme.ini",TheGlobalData->getPath_UserData().str());
//		if	(FindFirstFile(userDataPath.str(), &findData) !=INVALID_HANDLE_VALUE)
//			ini.load(userDataPath,  INI_LOAD_OVERWRITE, NULL );
//	}
	if( m_schemeList.size() <= 0 )
	{
		DEBUG_ASSERTCRASH(FALSE,("There's no ControlBarScheme in the ControlBarSchemeList:m_schemeList that was just read from the INI file"));
		return;
	}

}

//
// only recommended to use if you know what you're doing, cause you could actually load in
// a control bar built for 16x12 into an 8x6 screen which would use a lot more memory then it should
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::setControlBarScheme(AsciiString schemeName)
{
	ControlBarScheme *tempScheme = findControlBarScheme( schemeName );
	if(tempScheme)
	{
		// setup the multiplyer value
		m_multiplyer.x = TheDisplay->getWidth() / tempScheme->m_ScreenCreationRes.x;
		m_multiplyer.y = TheDisplay->getHeight() / tempScheme->m_ScreenCreationRes.y;
		m_currentScheme = tempScheme;
	}
	else
	{
		DEBUG_ASSERTCRASH(FALSE,("There's no ControlBarScheme in the ControlBarSchemeList:m_schemeList"));
		m_currentScheme = NULL;
	}
	if(m_currentScheme)
		m_currentScheme->init();
}

//
// Update calls all the animation update calls for the
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::update( void )
{
	if(m_currentScheme)	
		m_currentScheme->update();
}

//-----------------------------------------------------------------------------
void ControlBarSchemeManager::drawForeground( ICoord2D offset )
{
	if(m_currentScheme)	
		m_currentScheme->drawForeground( m_multiplyer, offset);
}
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::drawBackground( ICoord2D offset )
{
	if(m_currentScheme)	
		m_currentScheme->drawBackground( m_multiplyer, offset );
}

//-----------------------------------------------------------------------------
void ControlBarSchemeManager::setControlBarSchemeByPlayerTemplate( const PlayerTemplate *pt, Bool useSmall)
{
	if(!pt)
		return;
	AsciiString side = pt->getSide();
	if(useSmall)
		side.concat("Small");
	if(m_currentScheme && (m_currentScheme->m_side.compare(side) == 0))
	{
		m_currentScheme->init();

		DEBUG_LOG(("setControlBarSchemeByPlayer already is using %s as its side\n", side.str()));
		return;
	}

	// if we don't have a side, set it to Observer shell
	if(side.isEmpty())
		side.set("Observer");
	DEBUG_LOG(("setControlBarSchemeByPlayer used %s as its side\n", side.str()));
	ControlBarScheme *tempScheme = NULL;

	ControlBarSchemeList::iterator it = m_schemeList.begin();
	
	// iterate through the list and return the scheme that we're looking for
	while (it != m_schemeList.end())
	{
		ControlBarScheme *CBScheme = *it;
		if( !CBScheme )
		{
			DEBUG_ASSERTCRASH(FALSE,("There's no ControlBarScheme in the ControlBarSchemeList:m_schemeList"));
			it++;
			continue;
		}
		// find the scheme that best matches our resolution
		if(CBScheme->m_side.compareNoCase( side ) == 0)
		{
		
			if((!tempScheme || tempScheme->m_ScreenCreationRes.x < CBScheme->m_ScreenCreationRes.x) )//&& TheDisplay->getWidth() >= CBScheme->m_ScreenCreationRes.x )
				tempScheme = CBScheme;
		}
		it ++;	
	}
	
	if(tempScheme)
	{
		// setup the multiplyer value
 		m_multiplyer.x = TheDisplay->getWidth() / (Real)tempScheme->m_ScreenCreationRes.x;
		m_multiplyer.y = TheDisplay->getHeight() / (Real)tempScheme->m_ScreenCreationRes.y;
		m_currentScheme = tempScheme;
	}
	else
	{
		// well, we couldn't find
		m_currentScheme = findControlBarScheme("Default");
		DEBUG_LOG(("There's no ControlBarScheme with a side of %s", side.str()));
//		m_currentScheme = NULL;
	}
	if(m_currentScheme)
		m_currentScheme->init();
}
//-----------------------------------------------------------------------------
void ControlBarSchemeManager::setControlBarSchemeByPlayer(Player *p)
{
	GameWindow *communicatorButton = TheWindowManager->winGetWindowFromId( NULL, NAMEKEY("ControlBar.wnd:PopupCommunicator") );
	if (communicatorButton && TheControlBar)
	{
		if (TheRecorder->isMultiplayer())
			TheControlBar->setControlCommand(communicatorButton, TheControlBar->findCommandButton("NonCommand_Communicator") );
		else
			TheControlBar->setControlCommand(communicatorButton, TheControlBar->findCommandButton("NonCommand_BriefingHistory") );
	}

	if(!p)
		return;
	AsciiString side = p->getSide();
	if(m_currentScheme && (m_currentScheme->m_side.compare(side) == 0))
	{
		m_currentScheme->init();

		DEBUG_LOG(("setControlBarSchemeByPlayer already is using %s as its side\n", side.str()));
		return;
	}

	// if we don't have a side, set it to Observer shell
	if(side.isEmpty())
		side.set("Observer");
	DEBUG_LOG(("setControlBarSchemeByPlayer used %s as its side\n", side.str()));
	ControlBarScheme *tempScheme = NULL;

	ControlBarSchemeList::iterator it = m_schemeList.begin();
	
	// iterate through the list and return the scheme that we're looking for
	while (it != m_schemeList.end())
	{
		ControlBarScheme *CBScheme = *it;
		if( !CBScheme )
		{
			DEBUG_ASSERTCRASH(FALSE,("There's no ControlBarScheme in the ControlBarSchemeList:m_schemeList"));
			it++;
			continue;
		}
		// find the scheme that best matches our resolution
		if(CBScheme->m_side.compareNoCase( side ) == 0)
		{
		
			if((!tempScheme || tempScheme->m_ScreenCreationRes.x < CBScheme->m_ScreenCreationRes.x) )//&& TheDisplay->getWidth() >= CBScheme->m_ScreenCreationRes.x )
				tempScheme = CBScheme;
		}
		it ++;	
	}
	
	if(tempScheme)
	{
		// setup the multiplyer value
 		m_multiplyer.x = TheDisplay->getWidth() / (Real)tempScheme->m_ScreenCreationRes.x;
		m_multiplyer.y = TheDisplay->getHeight() / (Real)tempScheme->m_ScreenCreationRes.y;
		m_currentScheme = tempScheme;
	}
	else
	{
		// well, we couldn't find
		m_currentScheme = findControlBarScheme("Default");
		DEBUG_LOG(("There's no ControlBarScheme with a side of %s", side.str()));
//		m_currentScheme = NULL;
	}
	if(m_currentScheme)
		m_currentScheme->init();
}	


//-----------------------------------------------------------------------------
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
static void animSlideRight( ControlBarSchemeAnimation *anim )
{
	if(!anim->m_animImage || (anim->m_animDuration == 0))
		return;

	UnsignedInt currentFrame = anim->getCurrentFrame();
	ICoord2D startPos = anim->getStartPos();
	// if we're at the end, bring us to the beginning
	if(currentFrame == anim->m_animDuration)
	{
		anim->m_animImage->m_position.x = startPos.x;
		anim->m_animImage->m_position.y = startPos.y;
		anim->setCurrentFrame( 0 );
		return;
	}
	else if(currentFrame == 0)
	{
		// if we're at the beginning, save off the start position
		startPos.x = anim->m_animImage->m_position.x;
		startPos.y = anim->m_animImage->m_position.y;
		anim->setStartPos(startPos);
	}
	
	// now lets animate this bad boy!
	
	// now increment the frame
	currentFrame++;
	anim->setCurrentFrame(currentFrame);

	// now lets find what position we should be at.
	anim->m_animImage->m_position.x = startPos.x + (((anim->m_finalPos.x - startPos.x) * currentFrame) / anim->m_animDuration);


}

