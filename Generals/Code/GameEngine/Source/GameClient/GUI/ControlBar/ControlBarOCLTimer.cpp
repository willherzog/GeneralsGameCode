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

// FILE: ControlBarOCLTimer.cpp //////////////////////////////////////////////////////////
// Author: Colin Day, March 2002
// Desc:   Methods specific to the control bar OCL Timer context
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/NameKeyGenerator.h"
#include "Common/ThingTemplate.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/OCLUpdate.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameText.h"
#include "GameClient/ControlBar.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/GadgetProgressBar.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ControlBar::updateOCLTimerTextDisplay( UnsignedInt totalSeconds, Real percent )
{
	UnicodeString text;
	static UnsignedInt descID = TheNameKeyGenerator->nameToKey( "ControlBar.wnd:OCLTimerStaticText" );
	GameWindow *descWindow = TheWindowManager->winGetWindowFromId( NULL, descID );

	static UnsignedInt barID = TheNameKeyGenerator->nameToKey( "ControlBar.wnd:OCLTimerProgressBar" );
	GameWindow *barWindow = TheWindowManager->winGetWindowFromId( NULL, barID );

	// santiy
	DEBUG_ASSERTCRASH( descWindow, ("Under construction window not found") );

	Int minutes = totalSeconds / 60;
	Int seconds = totalSeconds - (minutes * 60);

	// format the message
	if( seconds < 10 )
		text.format( TheGameText->fetch( "CONTROLBAR:OCLTimerDescWithPadding" ), minutes, seconds );
	else
		text.format( TheGameText->fetch( "CONTROLBAR:OCLTimerDesc" ), minutes, seconds );

	GadgetStaticTextSetText( descWindow, text );
	GadgetProgressBarSetProgress(barWindow, (percent * 100));

	// record this as the last time displayed
	m_displayedOCLTimerSeconds = totalSeconds;

}  // end updateOCLTimerTextDisplay

//-------------------------------------------------------------------------------------------------
/** Populate the interface for an OCL Timer context. */
//-------------------------------------------------------------------------------------------------
void ControlBar::populateOCLTimer( Object *creatorObject )
{

	// sanity
	if( creatorObject == NULL )
		return;

	// get our parent window
	GameWindow *parent = m_contextParent[ CP_OCL_TIMER ];

	// set the sell button
/// @todo srj -- remove hard-coding here, please
	const CommandButton *commandButton = findCommandButton( "Command_Sell" );
	NameKeyType id;
	id = TheNameKeyGenerator->nameToKey( "ControlBar.wnd:OCLTimerSellButton" );
	GameWindow *win = TheWindowManager->winGetWindowFromId( parent, id );
	setControlCommand( win, commandButton );
	win->winSetStatus( WIN_STATUS_USE_OVERLAY_STATES );


	// set the text percent and bar of our timer we are displaying
	updateContextOCLTimer( );

	// set the portrait for the thing being constructed
	setPortraitByObject( creatorObject );

}  // end populateUnderConstruction

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ControlBar::updateContextOCLTimer( void )
{
	Object *obj = m_currentSelectedDrawable->getObject();

	static const NameKeyType key_OCLUpdate = NAMEKEY( "OCLUpdate" );
	OCLUpdate *update = (OCLUpdate*)obj->findUpdateModule( key_OCLUpdate );

	UnsignedInt frames = update->getRemainingFrames();
	UnsignedInt seconds = frames / LOGICFRAMES_PER_SECOND;

	Real percent = update->getCountdownPercent();

	// if the time has changed since what was last shown to the user update the text
	if( m_displayedOCLTimerSeconds != seconds )
		updateOCLTimerTextDisplay( seconds, percent );

}  // end updatecontextUnderConstruction
