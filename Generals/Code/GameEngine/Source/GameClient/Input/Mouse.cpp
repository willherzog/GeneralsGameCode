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

// FILE: Mouse.cpp ////////////////////////////////////////////////////////////////////////////////
// Created:   Colin Day, June 2001
// Desc:      Basic mouse interactions
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Debug.h"
#include "Common/MessageStream.h"
#include "Common/GameEngine.h"
#include "Common/GlobalData.h"
#include "Common/INI.h"

#include "GameClient/Display.h"
#include "GameClient/DisplayStringManager.h"
#include "GameClient/GameClient.h"
#include "GameClient/GameText.h"
#include "GameClient/GameWindow.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Keyboard.h"
#include "GameClient/Mouse.h"
#include "GameClient/GlobalLanguage.h"

#include "GameLogic/ScriptEngine.h"


// PUBLIC DATA ////////////////////////////////////////////////////////////////////////////////////
Mouse *TheMouse = NULL;

const char *Mouse::RedrawModeName[RM_MAX] = {
	"Mouse:Windows",
	"Mouse:W3D",
	"Mouse:Poly",
	"Mouse:DX8",
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static const FieldParse TheMouseCursorFieldParseTable[] = 
{
	{ "CursorText",						INI::parseAsciiString,	NULL, offsetof( CursorInfo, cursorText ) },
	{ "CursorTextColor",			INI::parseRGBAColorInt,	NULL, offsetof( CursorInfo, cursorTextColor ) },
	{ "CursorTextDropColor",	INI::parseRGBAColorInt,	NULL, offsetof( CursorInfo, cursorTextDropColor ) },
	{ "W3DModel",							INI::parseAsciiString,	NULL,	offsetof( CursorInfo, W3DModelName ) },
	{ "W3DAnim",							INI::parseAsciiString,	NULL,	offsetof( CursorInfo, W3DAnimName ) },
	{ "W3DScale",							INI::parseReal,					NULL,	offsetof( CursorInfo, W3DScale ) },
	{ "Loop",									INI::parseBool,					NULL,	offsetof( CursorInfo, loop ) },
	{ "Image",							INI::parseAsciiString,	NULL,	offsetof( CursorInfo, imageName ) },
	{ "Texture",							INI::parseAsciiString,	NULL,	offsetof( CursorInfo, textureName ) },
	{ "HotSpot",							INI::parseICoord2D,	NULL,	offsetof( CursorInfo, hotSpotPosition ) },
	{ "Frames",							INI::parseInt,	NULL,	offsetof( CursorInfo, numFrames ) },
	{ "FPS",							INI::parseReal, NULL, offsetof( CursorInfo, fps)},
	{ "Directions",							INI::parseInt,	NULL,	offsetof( CursorInfo, numDirections ) },
};

static const FieldParse TheMouseFieldParseTable[] = 
{
	{ "TooltipFontName",						INI::parseAsciiString,NULL,			offsetof( Mouse, m_tooltipFontName ) },
	{ "TooltipFontSize",						INI::parseInt,				NULL,			offsetof( Mouse, m_tooltipFontSize ) },
	{ "TooltipFontIsBold",					INI::parseBool,				NULL,			offsetof( Mouse, m_tooltipFontIsBold ) },
	{ "TooltipAnimateBackground",		INI::parseBool,				NULL,			offsetof( Mouse, m_tooltipAnimateBackground ) },
	{ "TooltipFillTime",						INI::parseInt,				NULL,			offsetof( Mouse, m_tooltipFillTime ) },
	{ "TooltipDelayTime",						INI::parseInt,				NULL,			offsetof( Mouse, m_tooltipDelayTime ) },
	{ "TooltipTextColor",						INI::parseRGBAColorInt,	NULL,		offsetof( Mouse, m_tooltipColorText ) },
	{ "TooltipHighlightColor",			INI::parseRGBAColorInt,	NULL,		offsetof( Mouse, m_tooltipColorHighlight ) },
	{ "TooltipShadowColor",					INI::parseRGBAColorInt,	NULL,		offsetof( Mouse, m_tooltipColorShadow ) },
	{ "TooltipBackgroundColor",			INI::parseRGBAColorInt,	NULL,		offsetof( Mouse, m_tooltipColorBackground ) },
	{ "TooltipBorderColor",					INI::parseRGBAColorInt,	NULL,		offsetof( Mouse, m_tooltipColorBorder ) },
	{ "TooltipWidth",								INI::parsePercentToReal,NULL,		offsetof( Mouse, m_tooltipWidth ) },
	{ "CursorMode",									INI::parseInt,					NULL,		offsetof( Mouse, m_currentRedrawMode ) },
	{ "UseTooltipAltTextColor",			INI::parseBool,					NULL,		offsetof( Mouse, m_useTooltipAltTextColor ) },
	{ "UseTooltipAltBackColor",			INI::parseBool,					NULL,		offsetof( Mouse, m_useTooltipAltBackColor ) },
	{ "AdjustTooltipAltColor",			INI::parseBool,					NULL,		offsetof( Mouse, m_adjustTooltipAltColor ) },
	{ "OrthoCamera",								INI::parseBool,					NULL,		offsetof( Mouse, m_orthoCamera ) },
	{ "OrthoZoom",									INI::parseReal,					NULL,		offsetof( Mouse, m_orthoZoom ) },
	{ "DragTolerance",							INI::parseUnsignedInt,	NULL,		offsetof( Mouse, m_dragTolerance) },
	{ "DragTolerance3D",						INI::parseUnsignedInt,	NULL,		offsetof( Mouse, m_dragTolerance3D) },
	{ "DragToleranceMS",						INI::parseUnsignedInt,	NULL,		offsetof( Mouse, m_dragToleranceMS) },

};

///////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
/** Move the mouse in either relative or absolute coords */
//-------------------------------------------------------------------------------------------------
void Mouse::moveMouse( Int x, Int y, Int relOrAbs )
{

	if( relOrAbs == MOUSE_MOVE_RELATIVE )
	{
		m_currMouse.pos.x += x;
		m_currMouse.pos.y += y;
	}
	else
	{
		m_currMouse.pos.x = x;
		m_currMouse.pos.y = y;
	}

	if( m_currMouse.pos.x > m_maxX )
		m_currMouse.pos.x = m_maxX;
	else if( m_currMouse.pos.x < m_minX )
		m_currMouse.pos.x = m_minX;

	if( m_currMouse.pos.y > m_maxY )
		m_currMouse.pos.y = m_maxY;
	else if( m_currMouse.pos.y < m_minY )
		m_currMouse.pos.y = m_minY;

}  // end moveMouse

//-------------------------------------------------------------------------------------------------
/** Get the current information for the mouse from the device */
//-------------------------------------------------------------------------------------------------
void Mouse::updateMouseData( )
{
	static Bool busy = FALSE;
	Int index = 0;
	UnsignedByte result;

	// prevent reentrancy in the event we make this mouse multi-threaded
	if( busy == FALSE )
	{

		busy = TRUE;

		// Get latest mouse events from DirectX
		do
		{
			do
			{
				result = getMouseEvent( &m_mouseEvents[ index ], TRUE );
			}
			while( result == MOUSE_LOST );
			index++;
		}
		while( (result != MOUSE_NONE) &&
					 (index < sizeof( m_mouseEvents ) / sizeof( MouseIO )) );

		busy = FALSE;

	}  // end if

	if( index > 0 )
		m_eventsThisFrame = index - 1;
	else
		m_eventsThisFrame = 0;

	if( index != 0 )
		m_deadInputFrame = m_inputFrame;

}  // end updateMouseData

//-------------------------------------------------------------------------------------------------
/** Combine mouse events into the main mouse variables */
//-------------------------------------------------------------------------------------------------
void Mouse::processMouseEvent( Int index )
{
	Int movementType;

	m_currMouse.leftEvent = MOUSE_EVENT_NONE;
	m_currMouse.rightEvent = MOUSE_EVENT_NONE;
	m_currMouse.middleEvent = MOUSE_EVENT_NONE;
	m_currMouse.wheelPos = 0;

	// what type of movement commands are we setup for
	if( m_inputMovesAbsolute == TRUE )
		movementType = MOUSE_MOVE_ABSOLUTE;
	else
		movementType = MOUSE_MOVE_RELATIVE;

	// set the time of this event to the correct time
	m_currMouse.time = m_mouseEvents[ index ].time;

	if( index == 0 )
		checkForDrag();

	// add Mouse Position Changes to Master Position
	moveMouse( m_mouseEvents[ index ].pos.x, 
						 m_mouseEvents[ index ].pos.y, 
						 movementType );

	// Cumulate Wheel Adjustments
	m_currMouse.wheelPos += m_mouseEvents[ index ].wheelPos;

	// Check Left Mouse State
	if( m_mouseEvents[ index ].leftFrame )
	{
		if( m_currMouse.leftState != m_mouseEvents[ index ].leftState )
		{
			// State Change
			if( m_mouseEvents[ index ].leftState == MBS_Down )
			{
				// Mouse Down
				m_currMouse.leftEvent = GWM_LEFT_DOWN;
				m_currMouse.leftState = MBS_Down;
				m_currMouse.leftFrame = m_inputFrame;
			}
			else if ( m_mouseEvents[ index ].leftState == MBS_DoubleClick )
			{
				// Mouse Double Click
				m_currMouse.leftEvent = GWM_LEFT_DOUBLE_CLICK;
				m_currMouse.leftState = MBS_DoubleClick;
				m_currMouse.leftFrame = m_inputFrame;
			}
			else
			{
				// Mouse Up
				m_currMouse.leftEvent = GWM_LEFT_UP;
				m_currMouse.leftState = MBS_Up;
				m_currMouse.leftFrame = m_inputFrame;
			}
		}
	}
	else if( m_currMouse.leftState != MBS_Up && 
					(	(m_prevMouse.leftEvent == GWM_LEFT_DOWN) ||
						(m_prevMouse.leftEvent == GWM_LEFT_DRAG) ) )
	{
		m_currMouse.leftEvent = GWM_LEFT_DRAG;
	}

	// Check Right Mouse State
	if( m_mouseEvents[ index ].rightFrame )
	{
		if( m_currMouse.rightState != m_mouseEvents[ index ].rightState )
		{
			// State Change
			if( m_mouseEvents[ index ].rightState == MBS_Down )
			{
				// Mouse Down
				m_currMouse.rightEvent = GWM_RIGHT_DOWN;
				m_currMouse.rightState = MBS_Down;
				m_currMouse.rightFrame = m_inputFrame;
			}
			else if( m_mouseEvents[ index ].rightState == MBS_DoubleClick )
			{
				// Mouse Double Click
				m_currMouse.rightEvent = GWM_RIGHT_DOUBLE_CLICK;
				m_currMouse.rightState = MBS_DoubleClick;
				m_currMouse.rightFrame = m_inputFrame;
			}
			else
			{
				// Mouse Up
				m_currMouse.rightEvent = GWM_RIGHT_UP;
				m_currMouse.rightState = MBS_Up;
				m_currMouse.rightFrame = m_inputFrame;
			}
		}
	}
	else if( m_currMouse.rightState != MBS_Up && 
					(	(m_prevMouse.rightEvent == GWM_RIGHT_DOWN) ||
						(m_prevMouse.rightEvent == GWM_RIGHT_DRAG) ) )
	{
		m_currMouse.rightEvent = GWM_RIGHT_DRAG;
	}

	// Check Middle Mouse State
	if( m_mouseEvents[ index ].middleFrame )
	{
		if( m_currMouse.middleState != m_mouseEvents[index].middleState )
		{
			// State Change
			if( m_mouseEvents[index].middleState == MBS_Down )
			{
				m_currMouse.middleEvent = GWM_MIDDLE_DOWN;
				m_currMouse.middleState = MBS_Down;
				m_currMouse.middleFrame = m_inputFrame;
			}
			else if( m_mouseEvents[index].middleState == MBS_DoubleClick )
			{
				m_currMouse.middleEvent = GWM_MIDDLE_DOUBLE_CLICK;
				m_currMouse.middleState = MBS_DoubleClick;
				m_currMouse.middleFrame = m_inputFrame;
			}
			else
			{
				// Mouse Up
				m_currMouse.middleEvent = GWM_MIDDLE_UP;
				m_currMouse.middleState = MBS_Up;
				m_currMouse.middleFrame = m_inputFrame;
			}
		}
	}
	else if( m_currMouse.middleState != MBS_Up && 
					(	(m_prevMouse.middleEvent == GWM_MIDDLE_DOWN) ||
						(m_prevMouse.middleEvent == GWM_MIDDLE_DRAG) ) )
	{
		m_currMouse.middleEvent = GWM_MIDDLE_DRAG;
	}

	m_currMouse.deltaPos.x = m_currMouse.pos.x - m_prevMouse.pos.x;
	m_currMouse.deltaPos.y = m_currMouse.pos.y - m_prevMouse.pos.y;
//	DEBUG_LOG(("Mouse dx %d, dy %d, index %d, frame %d", m_currMouse.deltaPos.x, m_currMouse.deltaPos.y, index, m_inputFrame));
//	// check if mouse is still and flag tooltip
//	if( ((dx*dx) + (dy*dy)) < CURSOR_MOVE_TOL_SQ )
//	{
//		// cursor is still
//		//m_stillTime++;
//    Int delay = m_tooltipDelayTime;
//    if(m_tooltipDelay >= 0 )
//       delay = m_tooltipDelay;
//    
//		if( now - m_stillTime >= delay )
//		{
//			if (!m_displayTooltip)
//			{
//				m_highlightPos = 0;
//				m_highlightUpdateStart = timeGetTime();
//			}
//
//			// display tooltip for current window
//			m_displayTooltip = TRUE;
//		}
//	}
//	else
//	{
//		// cursor moved
//		//m_stillTime = 0;
//		m_stillTime = now;
//		m_displayTooltip = FALSE;
//	}


	m_prevMouse = m_currMouse;

}  // end processMouseEvent

//-------------------------------------------------------------------------------------------------
/** Check for mouse drag */
//-------------------------------------------------------------------------------------------------
void Mouse::checkForDrag( void )
{

	if( m_currMouse.leftState && 
			(	(m_prevMouse.leftEvent == GWM_LEFT_DOWN) ||
				(m_prevMouse.leftEvent == GWM_LEFT_DRAG) ) )
	{
		m_currMouse.leftEvent = GWM_LEFT_DRAG;
	}

	if( m_currMouse.rightState && 
			(	(m_prevMouse.rightEvent == GWM_RIGHT_DOWN) ||
				(m_prevMouse.rightEvent == GWM_RIGHT_DRAG) ) )
	{
		m_currMouse.rightEvent = GWM_RIGHT_DRAG;
	}

	if( m_currMouse.middleState && 
			(	(m_prevMouse.middleEvent == GWM_MIDDLE_DOWN) ||
				(m_prevMouse.middleEvent == GWM_MIDDLE_DRAG) ) )
	{
		m_currMouse.middleEvent = GWM_MIDDLE_DRAG;
	}

}  // end checkForDrag


//-------------------------------------------------------------------------------------------------
/** Check for mouse click, using allowed drag forgiveness */
//-------------------------------------------------------------------------------------------------
Bool Mouse::isClick(const ICoord2D *anchor, const ICoord2D *dest, UnsignedInt previousMouseClick, UnsignedInt currentMouseClick)
{
	ICoord2D delta;
	delta.x = anchor->x - dest->x;
	delta.y = anchor->y - dest->y;


	// if the mouse hasn't moved further than the tolerance distance
	// or the click took less than the tolerance duration
	if (	abs(delta.x) > m_dragTolerance
		||	abs(delta.y) > m_dragTolerance
		||	currentMouseClick - previousMouseClick > m_dragToleranceMS)
	{
		return FALSE;
	}
	return TRUE;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CursorInfo::CursorInfo( void )
{
	// Added Sadullah Nader
	// Initializations missing and needed
	
	cursorName.clear();
	cursorText.clear();
	cursorTextColor.red = cursorTextColor.green = cursorTextColor.blue = 0;
	cursorTextDropColor.red = cursorTextDropColor.blue = cursorTextDropColor.green = 0;
	 
	//
	textureName.clear();
	imageName.clear();
	W3DModelName.clear();
	W3DAnimName.clear();
	W3DScale = 1.0f;
	loop = TRUE;
	//Assume hotspot is at the center of a 32x32 image.
	hotSpotPosition.x=16;
	hotSpotPosition.y=16;
	numFrames = 1;	//assume no animation
	fps=20.0f;
	numDirections=1;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Mouse::Mouse( void )
{
	// device info
	m_numButtons = 0;
	m_numAxes = 0;
	m_forceFeedback = FALSE;
	
	//Added By Sadullah Nader
	//Initializations missing and needed
	m_dragTolerance = 0;
	m_dragTolerance3D = 0;
	m_dragToleranceMS = 0;
	//
	
	//m_tooltipString.clear();	// redundant
	m_displayTooltip = FALSE;
	m_tooltipDisplayString = NULL;
  m_tooltipDelay = -1;  // default value
	// initialize all the mouse io data
	memset( m_mouseEvents, 0, sizeof( m_mouseEvents ) );
	memset( &m_currMouse, 0, sizeof( m_currMouse ) );
	memset( &m_prevMouse, 0, sizeof( m_prevMouse ) );

	m_minX = 0;
	m_maxX = 0;
	m_minY = 0;
	m_maxY = 0;
	m_eventsThisFrame = 0;

	m_inputFrame = 0;
	m_deadInputFrame =0;

	m_inputMovesAbsolute = FALSE;

	m_currentCursor = ARROW;
	if (TheGlobalData && TheGlobalData->m_winCursors)
		m_currentRedrawMode = RM_WINDOWS;
	else
		m_currentRedrawMode = RM_W3D;//RM_WINDOWS;
	m_visible = FALSE;
	m_tooltipFontName = "Times New Roman";
	m_tooltipFontSize = 12;
	m_tooltipFontIsBold = FALSE;
	m_tooltipAnimateBackground = TRUE;
	m_tooltipFillTime = 50;
	m_tooltipDelayTime = 50;

#define setColor(x, r, g, b, a) { x.red = r; x.green = g; x.blue = b; x.alpha = a; }
	setColor(m_tooltipColorText, 220, 220, 220, 255);
	setColor(m_tooltipColorHighlight, 255, 255, 0, 255);
	setColor(m_tooltipColorShadow, 0, 0, 0, 255);
	setColor(m_tooltipColorBackground, 20, 20, 0, 127);
	setColor(m_tooltipColorBorder, 0, 0, 0, 255);
#undef setColor

	m_tooltipWidth = 15.0f;
	m_lastTooltipWidth = 0.0f;

	m_useTooltipAltTextColor = FALSE;
	m_useTooltipAltBackColor = FALSE;
	m_adjustTooltipAltColor = FALSE;

	m_orthoCamera = FALSE;
	m_orthoZoom = 1.0f;

	m_isTooltipEmpty = TRUE;

	m_cursorTextDisplayString = NULL;
	m_cursorTextColor.red   = 255;
	m_cursorTextColor.green = 255;
	m_cursorTextColor.blue  = 255;
	m_cursorTextColor.alpha = 255;
	m_cursorTextDropColor.red   = 255;
	m_cursorTextDropColor.green = 255;
	m_cursorTextDropColor.blue  = 255;
	m_cursorTextDropColor.alpha = 255;
	
	m_highlightPos = 0;
	m_highlightUpdateStart = 0;
	m_stillTime = 0;
	m_tooltipTextColor.red = 255;
	m_tooltipTextColor.green = 255;
	m_tooltipTextColor.blue = 255;
	m_tooltipTextColor.alpha = 255;
	m_tooltipBackColor.red = 0;
	m_tooltipBackColor.green = 0;
	m_tooltipBackColor.blue = 0;
	m_tooltipBackColor.alpha = 255;

}  // end Mouse

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Mouse::~Mouse( void )
{
	if(m_tooltipDisplayString)
		TheDisplayStringManager->freeDisplayString(m_tooltipDisplayString);
	m_tooltipDisplayString = NULL;

	if( m_cursorTextDisplayString )
		TheDisplayStringManager->freeDisplayString( m_cursorTextDisplayString );
	m_cursorTextDisplayString = NULL;

}  // end ~Mouse

/**Had to move this out of main init() because I need this data to properly initialize
the Win32 version of the mouse (by preloading resources before D3D device is created).*/
void Mouse::parseIni(void)
{
	INI ini;
	ini.load( AsciiString( "Data\\INI\\Mouse.ini" ), INI_LOAD_OVERWRITE, NULL );
}

//-------------------------------------------------------------------------------------------------
/** Initialize the mouse */
//-------------------------------------------------------------------------------------------------
void Mouse::init( void )
{
	if (TheGlobalData && TheGlobalData->m_winCursors)
		m_currentRedrawMode = RM_WINDOWS;

	// device info
	m_numButtons = 2;  // by default just have 2 buttons
	m_numAxes = 2;  // by default a normal mouse moves in a 2d plane
	m_forceFeedback = FALSE;
	mouseNotifyResolutionChange();
	m_tooltipString.clear();	// redundant
	m_displayTooltip = FALSE;

	// initialize all the mouse io data
	memset( m_mouseEvents, 0, sizeof( m_mouseEvents ) );
	memset( &m_currMouse, 0, sizeof( m_currMouse ) );
	memset( &m_prevMouse, 0, sizeof( m_prevMouse ) );

	m_minX = 0;
	m_maxX = 799;
	m_minY = 0;
	m_maxY = 599;

	m_inputFrame = 0;
	m_deadInputFrame =0;

	m_inputMovesAbsolute = FALSE;
	m_eventsThisFrame = 0;

	m_currentCursor = ARROW;

 	// allocate a new display string
	m_cursorTextDisplayString = TheDisplayStringManager->newDisplayString();

}  // end init

//-------------------------------------------------------------------------------------------------
/** Tell mouse system display resolution changed. */
//-------------------------------------------------------------------------------------------------
void Mouse::mouseNotifyResolutionChange( void )
{
	if(m_tooltipDisplayString)
		TheDisplayStringManager->freeDisplayString(m_tooltipDisplayString);
	m_tooltipDisplayString = NULL;

	m_tooltipDisplayString = TheDisplayStringManager->newDisplayString();

	if (TheGlobalLanguageData && TheGlobalLanguageData->m_tooltipFontName.name.isNotEmpty())
	{
		m_tooltipDisplayString->setFont( TheFontLibrary->getFont(
			TheGlobalLanguageData->m_tooltipFontName.name,
			TheGlobalLanguageData->adjustFontSize(TheGlobalLanguageData->m_tooltipFontName.size),
			TheGlobalLanguageData->m_tooltipFontName.bold) );
	}
	else
	{
		m_tooltipDisplayString->setFont( TheFontLibrary->getFont(
			m_tooltipFontName,
			TheGlobalLanguageData->adjustFontSize(m_tooltipFontSize),
			m_tooltipFontIsBold ) );
	}

	m_tooltipDisplayString->setWordWrap(120);

}  // end reset


//-------------------------------------------------------------------------------------------------
/** Reset mouse system */
//-------------------------------------------------------------------------------------------------
void Mouse::reset( void )
{

	///@ todo Write Mouse::reset() if there needs to be anything here

	// reset the text of the cursor text
  if ( m_cursorTextDisplayString )
  	m_cursorTextDisplayString->reset();

}  // end reset

//-------------------------------------------------------------------------------------------------
/** Update the states of the mouse position and buttons */
//-------------------------------------------------------------------------------------------------
void Mouse::update( void )
{

	// increment input frame
	m_inputFrame++;

	// update the mouse data
	updateMouseData( );

}  // end update

//-------------------------------------------------------------------------------------------------
/** Given the current state of this input device, turn the raw input
	* data into raw stream messages and place those messages on the stream.
	* NOTE that the click messages replace up messages in the mouse, so we
	* are going to propagate those click messages in addition to up messages */
//-------------------------------------------------------------------------------------------------
void Mouse::createStreamMessages( void )
{

	// santiy
	if( TheMessageStream == NULL )
		return;  // no place to put messages

	GameMessage *msg = NULL;
	UnsignedInt now = timeGetTime();

	// basic position messages are always created
	msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_POSITION );
	msg->appendPixelArgument( m_currMouse.pos );
	msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );

  Int delay = m_tooltipDelayTime;
  if(m_tooltipDelay >= 0 )
     delay = m_tooltipDelay;
	if( TheGlobalData->m_scriptDebug )
	{
		//No delay while scriptdebugging!
		delay = 0;
	}
  
	if( now - m_stillTime >= delay )
	{
		if (!m_displayTooltip)
		{
			m_highlightPos = 0;
			m_highlightUpdateStart = timeGetTime();
		}

		// display tooltip for current window
		m_displayTooltip = TRUE;
	}
	else
	{
		//DEBUG_LOG(("%d %d %d %d", TheGameClient->getFrame(), delay, now, m_stillTime));
		m_displayTooltip = FALSE;
	}
	
	for (Int i = 0; i < m_eventsThisFrame; ++i)
	{
		processMouseEvent(i);
		if (m_currMouse.deltaPos.x || m_currMouse.deltaPos.y)
			m_stillTime = now;

		// button messages
		msg = NULL;
		switch( m_currMouse.leftEvent )
		{

			case GWM_LEFT_DOWN:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_LEFT_BUTTON_DOWN );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_LEFT_DOUBLE_CLICK:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_LEFT_DOUBLE_CLICK );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_LEFT_UP:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_LEFT_BUTTON_UP );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_LEFT_DRAG:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_LEFT_DRAG );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendPixelArgument( m_currMouse.deltaPos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				break;

		}  // end switch

		msg = NULL;
		switch( m_currMouse.middleEvent )
		{

			//-------------------------------------------------------------------------
			case GWM_MIDDLE_DOWN:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_MIDDLE_BUTTON_DOWN );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_MIDDLE_DOUBLE_CLICK:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_MIDDLE_DOUBLE_CLICK );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_MIDDLE_UP:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_MIDDLE_BUTTON_UP );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_MIDDLE_DRAG:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_MIDDLE_DRAG );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendPixelArgument( m_currMouse.deltaPos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				break;

		}  // end switch

		msg = NULL;
		switch( m_currMouse.rightEvent )
		{

			//-------------------------------------------------------------------------
			case GWM_RIGHT_DOWN:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_DOWN );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_RIGHT_DOUBLE_CLICK:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_RIGHT_DOUBLE_CLICK );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_RIGHT_UP:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_RIGHT_BUTTON_UP );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				msg->appendIntegerArgument( m_currMouse.time );
				break;

			case GWM_RIGHT_DRAG:
				msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_RIGHT_DRAG );
				msg->appendPixelArgument( m_currMouse.pos );
				msg->appendPixelArgument( m_currMouse.deltaPos );
				msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
				break;

		}  // end switch

		// wheel pos
		msg = NULL;
		if( m_currMouse.wheelPos != 0 )
		{
			msg = TheMessageStream->appendMessage( GameMessage::MSG_RAW_MOUSE_WHEEL );
			msg->appendPixelArgument( m_currMouse.pos );
			msg->appendIntegerArgument( m_currMouse.wheelPos / 120 );  // wheel delta
			msg->appendIntegerArgument( TheKeyboard->getModifierFlags() );
		}  // end if
	
	}	// end for
}  // end createStreamMessages

//-------------------------------------------------------------------------------------------------
/** Set the string to display at the cursor for the tooltip */
//-------------------------------------------------------------------------------------------------
void Mouse::setCursorTooltip( UnicodeString tooltip, Int delay, const RGBColor *color, Real width )
{

	//DEBUG_LOG(("%d Tooltip: %ls", TheGameClient->getFrame(), tooltip.str()));

	m_isTooltipEmpty = tooltip.isEmpty();
  m_tooltipDelay = delay;

	Bool forceRecalc = FALSE;
	if ( !tooltip.isEmpty() && width != m_lastTooltipWidth )
	{
		forceRecalc = TRUE;
		Int widthInPixels = (Int)(TheDisplay->getWidth()*m_tooltipWidth*width);
		if (widthInPixels < 10)
		{
			widthInPixels = 120;
		}
		else if (widthInPixels > TheDisplay->getWidth())
		{
			widthInPixels = TheDisplay->getWidth();
		}
		//DEBUG_LOG(("Setting tooltip width to %d pixels (%g%% of the normal tooltip width)", widthInPixels, width*100));
		m_tooltipDisplayString->setWordWrap( widthInPixels );
		m_lastTooltipWidth = width;
	}

	if (forceRecalc || !m_isTooltipEmpty && tooltip.compare(m_tooltipDisplayString->getText()))
	{
		m_tooltipDisplayString->setText(tooltip);
		//DEBUG_LOG(("Tooltip: %ls", tooltip.str()));
	}
	if (color)
	{
		if (m_useTooltipAltTextColor)
		{
			if (m_adjustTooltipAltColor)
			{
				m_tooltipTextColor.red   = REAL_TO_INT((color->red + 1.0f)   * 255.0f / 2.0f);
				m_tooltipTextColor.green = REAL_TO_INT((color->green + 1.0f) * 255.0f / 2.0f);
				m_tooltipTextColor.blue  = REAL_TO_INT((color->blue + 1.0f)  * 255.0f / 2.0f);
			}
			else
			{
				m_tooltipTextColor.red   = REAL_TO_INT(color->red   * 255.0f);
				m_tooltipTextColor.green = REAL_TO_INT(color->green * 255.0f);
				m_tooltipTextColor.blue  = REAL_TO_INT(color->blue  * 255.0f);
			}
			m_tooltipTextColor.alpha = m_tooltipColorText.alpha;
		}
		if (m_useTooltipAltBackColor)
		{
			if (m_adjustTooltipAltColor)
			{
				m_tooltipBackColor.red   = REAL_TO_INT(color->red   * 255.0f * 0.5f);
				m_tooltipBackColor.green = REAL_TO_INT(color->green * 255.0f * 0.5f);
				m_tooltipBackColor.blue  = REAL_TO_INT(color->blue  * 255.0f * 0.5f);
			}
			else
			{
				m_tooltipBackColor.red   = REAL_TO_INT(color->red   * 255.0f);
				m_tooltipBackColor.green = REAL_TO_INT(color->green * 255.0f);
				m_tooltipBackColor.blue  = REAL_TO_INT(color->blue  * 255.0f);
			}
			m_tooltipBackColor.alpha = m_tooltipColorBackground.alpha;
		}
	}
	else
	{
		m_tooltipTextColor = m_tooltipColorText;
		m_tooltipBackColor = m_tooltipColorBackground;
	}

}  // end setCursorTooltip

// ------------------------------------------------------------------------------------------------
/** Set the text for the mouse cursor ... note that this is *NOT* the tooltip text we
	* can set to be at the mouse position */
// ------------------------------------------------------------------------------------------------
void Mouse::setMouseText( UnicodeString text, 
													const RGBAColorInt *color, 
													const RGBAColorInt *dropColor )
{

	// sanity, if no display string has been created, get out of here
	if( m_cursorTextDisplayString == NULL )
		return;

	// set the text into the cursor display string
	m_cursorTextDisplayString->setText( text );

	// save the colors to draw in
	if( color )
		m_cursorTextColor = *color;
	if( dropColor )
		m_cursorTextDropColor = *dropColor;

}  // end setMouseText

//-------------------------------------------------------------------------------------------------
/** Move the mouse to the position */
//-------------------------------------------------------------------------------------------------
void Mouse::setPosition( Int x, Int y )
{

	m_currMouse.pos.x = x;
	m_currMouse.pos.y = y;

}  // end setPosition

//-------------------------------------------------------------------------------------------------
/** This default implemtation of SetMouseLimits will just set the limiting	
	* rectangle to be the width and height of the game display with the
	* origin in the upper left at (0,0).  However, if the game is running
	* in a windowed mode then these limits should reflect the SCREEN
	* coords that the mouse is allowed to move in.  Also, if the game is in
	* a window you may want to adjust for any title bar available in
	* the operating system.  For system specific limits and windows etc,
	* just override this function in the device implementation of the mouse */
//-------------------------------------------------------------------------------------------------
void Mouse::setMouseLimits( void )
{

	m_minX = 0;
	m_minY = 0;
	if( TheDisplay )
	{

		m_maxX = TheDisplay->getWidth();
		m_maxY = TheDisplay->getHeight();
	
	}  // end if

}  // end setMouseLimits

//-------------------------------------------------------------------------------------------------
/** Draw the mouse */
//-------------------------------------------------------------------------------------------------
void Mouse::draw( void )
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void Mouse::resetTooltipDelay( void )
{
	m_stillTime = timeGetTime();
	m_displayTooltip = FALSE;
}

//-------------------------------------------------------------------------------------------------
/** Draw the mouse tooltip if one is set */
//-------------------------------------------------------------------------------------------------
void Mouse::drawTooltip( void )
{
	if (TheScriptEngine->getFade()!=ScriptEngine::FADE_NONE) {
		return;
	}

	/// @todo: Still need to put in display logic so it puts the tool tips in a visable position on the edge of the screen
	if( m_displayTooltip && TheDisplay && m_tooltipDisplayString && (m_tooltipDisplayString->getTextLength() > 0) && !m_isTooltipEmpty)
	{
		Int width, xPos;
		Int height, yPos;
		m_tooltipDisplayString->getSize(&width,&height);
		xPos = m_currMouse.pos.x + 20;
		yPos = m_currMouse.pos.y;// + 20;

		if( xPos + width + 4 > m_maxX ) // +4 for spill
		{
			//xPos = m_maxX - width;
			xPos -= 20 + width;
		}
		if( yPos + height + 4 > m_maxY ) // +4 for spill
		{
			//yPos = m_maxY - height;
			yPos -= /*40 +*/ height;
		}

		Int boxWidth = (m_tooltipAnimateBackground)?(min(width, m_highlightPos)):width;

#define GMC(x) GameMakeColor(x.red, x.green, x.blue, x.alpha)
#define COLOR(x) GMC(m_tooltipColor##x)
		TheDisplay->drawFillRect(xPos, yPos, boxWidth + 2,height + 2, GMC(m_tooltipBackColor));//GameMakeColor(0,0,0,125));
		TheDisplay->drawOpenRect(xPos, yPos, boxWidth + 2,height + 2, 1.0, COLOR(Border));//GameMakeColor(20,20,20,255));

		// build clip rect
		IRegion2D clipRegion;
		clipRegion.lo.x = xPos+2;
		clipRegion.lo.y = yPos+1;
		clipRegion.hi.x = xPos+2+m_highlightPos;
		clipRegion.hi.y = yPos+1+height;
		m_tooltipDisplayString->setClipRegion(&clipRegion);
		m_tooltipDisplayString->draw(xPos +2, yPos +1, GMC(m_tooltipTextColor), COLOR(Shadow));//GameMakeColor(220,220,220,255),GameMakeColor(20,20,20,125));

		// highlight section
		const Int HIGHLIGHT_WIDTH = 15;
		clipRegion.lo.x = xPos+2+m_highlightPos-HIGHLIGHT_WIDTH;
		clipRegion.lo.y = yPos+1;
		clipRegion.hi.x = xPos+2+m_highlightPos;
		clipRegion.hi.y = yPos+1+height;
		m_tooltipDisplayString->setClipRegion(&clipRegion);
		m_tooltipDisplayString->draw(xPos +2, yPos +1, COLOR(Highlight), COLOR(Shadow));//GameMakeColor(255,255,0,255),GameMakeColor(20,20,20,125));

		// get ready for the next part of the anim
		if (m_highlightPos < width + HIGHLIGHT_WIDTH)
		{
			UnsignedInt now = timeGetTime();
			m_highlightPos = (width*(now-m_highlightUpdateStart))/m_tooltipFillTime;
		}
	}  // end if


}  // end drawTooltip

// ------------------------------------------------------------------------------------------------
/** Draw the cursor text at the mouse position.  Note that this is *NOT* the tooltip text */
// ------------------------------------------------------------------------------------------------
void Mouse::drawCursorText( void )
{

	// sanity
	if( m_cursorTextDisplayString == NULL )
		return;

	// get the colors to draw the text in an acceptable format
	Color color, dropColor;
	color = GameMakeColor( m_cursorTextColor.red, 
												 m_cursorTextColor.green, 
												 m_cursorTextColor.blue, 
												 m_cursorTextColor.alpha );
	dropColor = GameMakeColor( m_cursorTextDropColor.red, 
														 m_cursorTextDropColor.green, 
														 m_cursorTextDropColor.blue, 
														 m_cursorTextDropColor.alpha );

	// get the size of the text to draw
	Int width, height;
	m_cursorTextDisplayString->getSize( &width, &height );

	// draw the text around the cursor position
	Int x, y;
	x = m_currMouse.pos.x - width / 2;
	y = m_currMouse.pos.y - height / 2;
	m_cursorTextDisplayString->draw( x, y, color, dropColor );

}  // end drawCursorText

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Int Mouse::getCursorIndex(const AsciiString& name)
{
	if (name.isEmpty())
		return INVALID_MOUSE_CURSOR;

	/** @todo This is silly to have to define these names from INI in the code ... 
		* that should be changed (CBD) */
	static const char *CursorININames[NUM_MOUSE_CURSORS] =
	{
		//"InvalidMouseCursor",  // this entry is not actually a mouse cursor, but just a
														 // reminder that it does exist
		"None",
		"Normal",
		"Arrow",
		"Scroll",
		"Target",
		"Move",
		"AttackMove",
		"AttackObj",
		"ForceAttackObj",
		"ForceAttackGround",
		"Build",
		"InvalidBuild",
		"GenericInvalid",
		"Select",
		"EnterFriendly",
		"EnterAggressive",
		"SetRallyPoint",
		"GetRepaired",
		"GetHealed",
		"DoRepair",
		"ResumeConstruction",
		"CaptureBuilding",
		"SnipeVehicle",
		"LaserGuidedMissiles",
		"TankHunterTNTAttack",
		"StabAttack",
		"PlaceRemoteCharge",
		"PlaceTimedCharge",
		"Defector",
#ifdef ALLOW_DEMORALIZE
		"Demoralize",
#endif
		"Dock",
#ifdef ALLOW_SURRENDER
		"PickUpPrisoner",
		"ReturnToPrison",
#endif
		"FireFlame",
#ifdef ALLOW_SURRENDER
		"FireTranqDarts",
		"FireStunBullets",
#endif
		"FireBomb",
		"PlaceBeacon",
		"DisguiseAsVehicle",
		"Waypoint",
		"OutRange",
		"StabAttackInvalid",
		"PlaceChargeInvalid",
		"Hack",
		"ParticleUplinkCannon",

	};

	for (Int i=0; i<NUM_MOUSE_CURSORS; ++i)
	{
		if (name.compareNoCase(CursorININames[i]) == 0)
			return i;
	}

	DEBUG_CRASH(( "Mouse::getCursorIndex - Invalid cursor name '%s'", name.str() ));
	return INVALID_MOUSE_CURSOR;

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void Mouse::setCursor( MouseCursor cursor )
{
	
	// if cursor has not changed do nothing
	if( m_currentCursor == cursor )
		return;

	// only if we have a display cursor do we do anything with mouse text
	if( m_cursorTextDisplayString != NULL )
	{
		CursorInfo *cursorInfo = &m_cursorInfo[ cursor ];

		//
		// if there is text present into the cursor info, set it into the mouse, otherwise
		// just clear out the text for the cursor text
		//
		if( cursorInfo->cursorText.isEmpty() == FALSE )
			setMouseText( TheGameText->fetch( cursorInfo->cursorText.str() ),
										 &(cursorInfo->cursorTextColor),
										 &(cursorInfo->cursorTextDropColor) );
		else
			setMouseText( UnicodeString( L"" ), NULL, NULL );

	}  // end if

}  // end setCursor

//-------------------------------------------------------------------------------------------------
/** Parse MouseCursor entry */
//-------------------------------------------------------------------------------------------------
void INI::parseMouseCursorDefinition( INI* ini )
{
	const char *c;
	AsciiString name;

	// read the name
	c = ini->getNextToken();
	name.set( c );	

	if( TheMouse )
	{
		Int index = TheMouse->getCursorIndex(name);
		if (index != Mouse::INVALID_MOUSE_CURSOR)
		{
			CursorInfo *cursorInfo = &(TheMouse->m_cursorInfo[index]);
			cursorInfo->cursorName = name;

			// parse the ini weapon definition
			ini->initFromINI( cursorInfo, TheMouseCursorFieldParseTable );
		}

	}

}

//-------------------------------------------------------------------------------------------------
/** Parse MouseCursor entry */
//-------------------------------------------------------------------------------------------------
void INI::parseMouseDefinition( INI* ini )
{
	if( TheMouse )
	{
		// parse the ini weapon definition
		ini->initFromINI( TheMouse, TheMouseFieldParseTable );
	}
}


