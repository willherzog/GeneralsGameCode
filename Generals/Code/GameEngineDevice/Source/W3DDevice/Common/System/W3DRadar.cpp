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

// FILE: W3DRadar.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, January 2002
// Desc:   W3D radar implementation, this has the necessary device dependent drawing
//				 necessary for the radar
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/AudioEventRTS.h"
#include "Common/Debug.h"
#include "Common/GlobalData.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"

#include "GameLogic/TerrainLogic.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"

#include "GameLogic/Module/StealthUpdate.h"

#include "GameClient/Color.h"
#include "GameClient/Display.h"
#include "GameClient/GameClient.h"
#include "GameClient/GameWindow.h"
#include "GameClient/Image.h"
#include "GameClient/Line2D.h"
#include "GameClient/TerrainVisual.h"
#include "GameClient/Water.h"
#include "W3DDevice/Common/W3DRadar.h"
#include "W3DDevice/GameClient/HeightMap.h"
#include "W3DDevice/GameClient/W3DShroud.h"
#include "WW3D2/texture.h"
#include "WW3D2/dx8caps.h"



// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
enum { OVERLAY_REFRESH_RATE = 6 };  ///< over updates once this many frames

//-------------------------------------------------------------------------------------------------
/** Is the point legal, that is, inside the resolution of the radar cells */
//-------------------------------------------------------------------------------------------------
inline Bool legalRadarPoint( Int px, Int py )
{

	if( px < 0 || py < 0 || px >= RADAR_CELL_WIDTH || py >= RADAR_CELL_HEIGHT )
		return FALSE;

	return TRUE;

}

//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static WW3DFormat findFormat(const WW3DFormat formats[])
{
	for( Int i = 0; formats[ i ] != WW3D_FORMAT_UNKNOWN; i++ )
	{

		if( DX8Wrapper::Get_Current_Caps()->Support_Texture_Format( formats[ i ] ) )
		{

			return formats[ i ];

		}  // end if

	}  // end for i
	DEBUG_CRASH(("WW3DRadar: No appropriate texture format") );
	return WW3D_FORMAT_UNKNOWN;
}

//-------------------------------------------------------------------------------------------------
/** Find the texture format we're going to use for the radar.  The texture format must
	* be supported by the hardware.  The "more preferred" formats appear at the top of
	* the format tables in order from most preferred to least preferred */
//-------------------------------------------------------------------------------------------------
void W3DRadar::initializeTextureFormats( void )
{
	const WW3DFormat terrainFormats[] =
	{
		WW3D_FORMAT_R8G8B8,
		WW3D_FORMAT_X8R8G8B8,
		WW3D_FORMAT_R5G6B5,
		WW3D_FORMAT_X1R5G5B5,
		WW3D_FORMAT_UNKNOWN				// keep this one last
	};
	const WW3DFormat overlayFormats[] =
	{
		WW3D_FORMAT_A8R8G8B8,
		WW3D_FORMAT_A4R4G4B4,
		WW3D_FORMAT_UNKNOWN				// keep this one last
	};
	const WW3DFormat shroudFormats[] =
	{
		WW3D_FORMAT_A8R8G8B8,
		WW3D_FORMAT_A4R4G4B4,
		WW3D_FORMAT_UNKNOWN				// keep this one last
	};

	// find a format for the terrain texture
	m_terrainTextureFormat = findFormat(terrainFormats);

	// find a format for the overlay texture
	m_overlayTextureFormat = findFormat(overlayFormats);

	// find a format for the shroud texture
	m_shroudTextureFormat = findFormat(shroudFormats);

}  // end initializeTextureFormats

//-------------------------------------------------------------------------------------------------
/** Delete resources used specifically in this W3D radar implemetation */
//-------------------------------------------------------------------------------------------------
void W3DRadar::deleteResources( void )
{

	//
	// delete terrain resources used
	//
	if( m_terrainTexture )
		m_terrainTexture->Release_Ref();
	m_terrainTexture = NULL;
	if( m_terrainImage )
		deleteInstance(m_terrainImage);
	m_terrainImage = NULL;

	//
	// delete overlay resources used
	//
	if( m_overlayTexture )
		m_overlayTexture->Release_Ref();
	m_overlayTexture = NULL;
	if( m_overlayImage )
		deleteInstance(m_overlayImage);
	m_overlayImage = NULL;

	//
	// delete shroud resources used
	//
	if( m_shroudTexture )
		m_shroudTexture->Release_Ref();
	m_shroudTexture = NULL;
	if( m_shroudImage )
		deleteInstance(m_shroudImage);
	m_shroudImage = NULL;

}  // end deleteResources

//-------------------------------------------------------------------------------------------------
/** Reconstruct the view box given the current camera settings */
//-------------------------------------------------------------------------------------------------
void W3DRadar::reconstructViewBox( void )
{
	Coord3D world[ 4 ];
	ICoord2D radar[ 4 ];
	Int i;

	// get the 4 points of the view corners in the 3D world at the average Z height in the map
	TheTacticalView->getScreenCornerWorldPointsAtZ( &world[ 0 ],
																									&world[ 1 ],
																									&world[ 2 ],
																									&world[ 3 ],
																									getTerrainAverageZ() );

	// convert each of the 4 points in the world to radar cell positions
	for( i = 0; i < 4; i++ )
	{

		// first convert to radar cells
 		radar[ i ].x = world[ i ].x / (m_mapExtent.width() / RADAR_CELL_WIDTH);
 		radar[ i ].y = world[ i ].y / (m_mapExtent.height() / RADAR_CELL_HEIGHT);

		//
		// store these points in the view box array which contains a first position
		// of (0,0) and then offsets for each additional entry point
		//
		if( i == 0 )
		{

			m_viewBox[ i ].x = 0;
			m_viewBox[ i ].y = 0;

		}  // end if
		else
		{

			m_viewBox[ i ].x = radar[ i ].x - radar[ i - 1 ].x;
			m_viewBox[ i ].y = radar[ i ].y - radar[ i - 1 ].y;

		}  // end else

	}  // end for i

	//
	// save the camera settings for this view box, we will need to make it again only
	// if some of these change
	//
	m_viewAngle = TheTacticalView->getAngle();
	Coord3D pos;
	TheTacticalView->getPosition( &pos );
	m_viewZoom = TheTacticalView->getZoom();
	m_reconstructViewBox = FALSE;

}  // end reconstructViewBox

//-------------------------------------------------------------------------------------------------
/** Convert radar position to actual pixel coord */
//-------------------------------------------------------------------------------------------------
void W3DRadar::radarToPixel( const ICoord2D *radar, ICoord2D *pixel,
														 Int radarUpperLeftX, Int radarUpperLeftY,
														 Int radarWidth, Int radarHeight )
{

	// sanity
	if( radar == NULL || pixel == NULL )
		return;

	pixel->x = (radar->x * radarWidth / RADAR_CELL_WIDTH) + radarUpperLeftX;
	// note the "inverted" y here to orient the way our world looks with +x=right and -y=down
	pixel->y = ((RADAR_CELL_HEIGHT - 1 - radar->y) * radarHeight / RADAR_CELL_HEIGHT) + radarUpperLeftY;

}  // end radarToPixel


//-------------------------------------------------------------------------------------------------
/** Draw a hero icon at a position, given radar box upper left location and dimensions.  */
//-------------------------------------------------------------------------------------------------
void W3DRadar::drawHeroIcon( Int pixelX, Int pixelY, Int width, Int height, const Coord3D *pos )
{
	// get the hero icon image
	static const Image *image = (Image *)TheMappedImageCollection->findImageByName("HeroReticle");
	if (image != NULL)
	{
		// convert world to radar coords
		ICoord2D ulRadar;
		ulRadar.x = pos->x / (m_mapExtent.width() / RADAR_CELL_WIDTH);
		ulRadar.y = pos->y / (m_mapExtent.height() / RADAR_CELL_HEIGHT);

		// convert radar to screen coords
		ICoord2D offsetScreen;
		radarToPixel( &ulRadar, &offsetScreen, pixelX, pixelY, width, height );

		// shift from an upper left to a center focus for the icon
		int iconWidth = image->getImageWidth();
		int iconHeight = image->getImageHeight();
		offsetScreen.x -= (iconWidth / 2) - 1;
		offsetScreen.y -= iconHeight / 2;

		// draw the icon
		TheDisplay->drawImage( image, offsetScreen.x , offsetScreen.y, offsetScreen.x + iconWidth, offsetScreen.y + iconHeight );
	}
}

//-------------------------------------------------------------------------------------------------
/** Draw a "box" into the texture passed in that represents the viewable area for
	* the tactical display into the game world */
//-------------------------------------------------------------------------------------------------
void W3DRadar::drawViewBox( Int pixelX, Int pixelY, Int width, Int height )
{
	ICoord2D ulScreen;
	ICoord2D ulRadar;
	Coord3D ulWorld;
	ICoord2D ulStart = { 0, 0 };
	ICoord2D start, end;
	ICoord2D clipStart, clipEnd;
	Real lineWidth = 1.0f;
	Color topColor = GameMakeColor( 225, 225, 0, 255 );
	Color bottomColor = GameMakeColor( 158, 158, 0, 255 );

	//
	// setup the clipping region ... note that this clipping region is not over just the
	// radar image area ... it's in the WHOLE window available for the radar
	//
	IRegion2D clipRegion;
	ICoord2D radarWindowSize, radarWindowScreenPos;
	m_radarWindow->winGetSize( &radarWindowSize.x, &radarWindowSize.y );
	m_radarWindow->winGetScreenPosition( &radarWindowScreenPos.x, &radarWindowScreenPos.y );
	clipRegion.lo.x = radarWindowScreenPos.x;
	clipRegion.lo.y = radarWindowScreenPos.y;
	clipRegion.hi.x = radarWindowScreenPos.x + radarWindowSize.x;
	clipRegion.hi.y = radarWindowScreenPos.y + radarWindowSize.y;

	// convert top left of screen into world position
	TheTacticalView->getOrigin( &ulScreen.x, &ulScreen.y );
	TheTacticalView->screenToWorldAtZ( &ulScreen, &ulWorld, getTerrainAverageZ() );

	// convert world to radar coords
 	ulRadar.x = ulWorld.x / (m_mapExtent.width() / RADAR_CELL_WIDTH);
 	ulRadar.y = ulWorld.y / (m_mapExtent.height() / RADAR_CELL_HEIGHT);

	//
	// convert radar point to actual pixel coords on the screen, shifted
	// into position on the radar for where the radar is drawn and the size of the
	// area that the radar is drawn in
	//
	radarToPixel( &ulRadar, &ulStart, pixelX, pixelY, width, height );

	//
	// using our view box offset array, convert each of those radar cell offset points
	// into screen pixels and draw the box.  The view box array is setup with the
	// first index containing (0,0) (the point we just converted in theory), with cell
	// offsets to each of the other corners in the following order
	// (upper left, upper right, lower right, lower left)
	//
	ICoord2D radar;

	// top line
	start = ulStart;
	radar.x = ulRadar.x + m_viewBox[ 1 ].x;
	radar.y = ulRadar.y + m_viewBox[ 1 ].y;
	radarToPixel( &radar, &end, pixelX, pixelY, width, height );
	if( ClipLine2D( &start, &end, &clipStart, &clipEnd, &clipRegion ) )
		TheDisplay->drawLine( clipStart.x, clipStart.y, clipEnd.x, clipEnd.y,
													lineWidth, topColor );

  // right line
	start = end;
	radar.x += m_viewBox[ 2 ].x;
	radar.y += m_viewBox[ 2 ].y;
	radarToPixel( &radar, &end, pixelX, pixelY, width, height );
	if( ClipLine2D( &start, &end, &clipStart, &clipEnd, &clipRegion ) )
		TheDisplay->drawLine( clipStart.x, clipStart.y, clipEnd.x, clipEnd.y,
													lineWidth, topColor, bottomColor );

  // bottom line
	start = end;
	radar.x += m_viewBox[ 3 ].x;
	radar.y += m_viewBox[ 3 ].y;
	radarToPixel( &radar, &end, pixelX, pixelY, width, height );
	if( ClipLine2D( &start, &end, &clipStart, &clipEnd, &clipRegion ) )
		TheDisplay->drawLine( clipStart.x, clipStart.y, clipEnd.x, clipEnd.y,
													lineWidth, bottomColor );

  // left line
	start = end;
	end = ulStart;
	if( ClipLine2D( &start, &end, &clipStart, &clipEnd, &clipRegion ) )
		TheDisplay->drawLine( clipStart.x, clipStart.y, clipEnd.x, clipEnd.y,
													lineWidth, bottomColor, topColor );

}  // end drawViewBox

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DRadar::drawSingleBeaconEvent( Int pixelX, Int pixelY, Int width, Int height, Int index )
{
	RadarEvent *event = &(m_event[index]);
	ICoord2D tri[ 3 ];
	ICoord2D start, end;
	Real angle, addAngle;
	Color startColor, endColor;
	Real lineWidth = 1.0f;
	UnsignedInt currentFrame = TheGameLogic->getFrame();
	UnsignedInt frameDiff;							// frames the event has been alive for
	Real maxEventSize = width / 10.0f;   // max size of the event marker
	Int minEventSize = 6;     // min size of the event marker
	Int eventSize;									 // current size of a marker to draw
	const Real TIME_FROM_FULL_SIZE_TO_SMALL_SIZE = LOGICFRAMES_PER_SECOND * 1.5;
	Real totalAnglesToSpin = 2.0f * PI;  ///< spin around this many angles going from big to small
	UnsignedByte r, g, b, a;

	// setup screen clipping region
	IRegion2D clipRegion;
	clipRegion.lo.x = pixelX;
	clipRegion.lo.y = pixelY;
	clipRegion.hi.x = pixelX + width;
	clipRegion.hi.y = pixelY + height;

	// get the difference in frame from the current frame to the frame we were created on
	frameDiff = currentFrame - event->createFrame;

	// compute the size of the event marker, it is largest when it starts and smallest at the end
	eventSize = REAL_TO_INT( maxEventSize * ( 1.0f - frameDiff / TIME_FROM_FULL_SIZE_TO_SMALL_SIZE) );;

	// we never let the event size get too small
	if( eventSize < minEventSize )
		eventSize = minEventSize;

	// compute how much "angle" we will add to each point to make it rotate as it's getting small
	addAngle = -totalAnglesToSpin * (frameDiff / TIME_FROM_FULL_SIZE_TO_SMALL_SIZE);

	// create a triangle around the event
	angle = 0.0f - addAngle;
	tri[ 0 ].x = REAL_TO_INT( (DOUBLE_TO_REAL( Cos( angle ) ) * eventSize) + event->radarLoc.x );
	tri[ 0 ].y = REAL_TO_INT( (DOUBLE_TO_REAL( Sin( angle ) ) * eventSize) + event->radarLoc.y );

	angle = 2.0f * PI / 3.0f - addAngle;
	tri[ 1 ].x = REAL_TO_INT( (DOUBLE_TO_REAL( Cos( angle ) ) * eventSize) + event->radarLoc.x );
	tri[ 1 ].y = REAL_TO_INT( (DOUBLE_TO_REAL( Sin( angle ) ) * eventSize) + event->radarLoc.y );

	angle = -2.0f * PI / 3.0f - addAngle;
	tri[ 2 ].x = REAL_TO_INT( (DOUBLE_TO_REAL( Cos( angle ) ) * eventSize) + event->radarLoc.x );
	tri[ 2 ].y = REAL_TO_INT( (DOUBLE_TO_REAL( Sin( angle ) ) * eventSize) + event->radarLoc.y );

	// translate radar coords to screen coords
	radarToPixel( &tri[ 0 ], &tri[ 0 ], pixelX, pixelY, width, height );
	radarToPixel( &tri[ 1 ], &tri[ 1 ], pixelX, pixelY, width, height );
	radarToPixel( &tri[ 2 ], &tri[ 2 ], pixelX, pixelY, width, height );

	//
	// make the colors we're going to use, when we're at our smallest size we will start to
	// fade the alpha away to transparent so that at our lifetime frame we are completely gone
	//

	// color 1 ------------------
	r = event->color1.red;
	g = event->color1.green;
	b = event->color1.blue;
	a = event->color1.alpha;
	if( currentFrame > event->fadeFrame )
	{

		a = REAL_TO_UNSIGNEDBYTE( (Real)a * (1.0f - (Real)(currentFrame - event->fadeFrame) /
																								(Real)(event->dieFrame - event->fadeFrame) ) );

	}  // end if
	startColor = GameMakeColor( r, g, b, a );

	// color 2 ------------------
	r = event->color2.red;
	g = event->color2.green;
	b = event->color2.blue;
	a = event->color2.alpha;
	if( currentFrame > event->fadeFrame )
	{

		a = REAL_TO_UNSIGNEDBYTE( (Real)a * (1.0f - (Real)(currentFrame - event->fadeFrame) /
																								(Real)(event->dieFrame - event->fadeFrame) ) );

	}  // end if
	endColor = GameMakeColor( r, g, b, a );

	// draw the lines
	if( ClipLine2D( &tri[ 0 ], &tri[ 1 ], &start, &end, &clipRegion ) )
		TheDisplay->drawLine( start.x, start.y, end.x, end.y, lineWidth, startColor, endColor );
	if( ClipLine2D( &tri[ 1 ], &tri[ 2 ], &start, &end, &clipRegion ) )
		TheDisplay->drawLine( start.x, start.y, end.x, end.y, lineWidth, startColor, endColor );
	if( ClipLine2D( &tri[ 2 ], &tri[ 0 ], &start, &end, &clipRegion ) )
		TheDisplay->drawLine( start.x, start.y, end.x, end.y, lineWidth, startColor, endColor );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DRadar::drawSingleGenericEvent( Int pixelX, Int pixelY, Int width, Int height, Int index )
{
	RadarEvent *event = &(m_event[index]);
	ICoord2D tri[ 3 ];
	ICoord2D start, end;
	Real angle, addAngle;
	Color startColor, endColor;
	Real lineWidth = 1.0f;
	UnsignedInt currentFrame = TheGameLogic->getFrame();
	UnsignedInt frameDiff;							// frames the event has been alive for
	Real maxEventSize = width / 2.0f;   // max size of the event marker
	Int minEventSize = 6;     // min size of the event marker
	Int eventSize;									 // current size of a marker to draw
	const Real TIME_FROM_FULL_SIZE_TO_SMALL_SIZE = LOGICFRAMES_PER_SECOND * 1.5;
	Real totalAnglesToSpin = 2.0f * PI;  ///< spin around this many angles going from big to small
	UnsignedByte r, g, b, a;

	// setup screen clipping region
	IRegion2D clipRegion;
	clipRegion.lo.x = pixelX;
	clipRegion.lo.y = pixelY;
	clipRegion.hi.x = pixelX + width;
	clipRegion.hi.y = pixelY + height;

	// get the difference in frame from the current frame to the frame we were created on
	frameDiff = currentFrame - event->createFrame;

	// compute the size of the event marker, it is largest when it starts and smallest at the end
	eventSize = REAL_TO_INT( maxEventSize * ( 1.0f - frameDiff / TIME_FROM_FULL_SIZE_TO_SMALL_SIZE) );;

	// we never let the event size get too small
	if( eventSize < minEventSize )
		eventSize = minEventSize;

	// compute how much "angle" we will add to each point to make it rotate as it's getting small
	addAngle = totalAnglesToSpin * (frameDiff / TIME_FROM_FULL_SIZE_TO_SMALL_SIZE);

	// create a triangle around the event
	angle = 0.0f - addAngle;
	tri[ 0 ].x = REAL_TO_INT( (DOUBLE_TO_REAL( Cos( angle ) ) * eventSize) + event->radarLoc.x );
	tri[ 0 ].y = REAL_TO_INT( (DOUBLE_TO_REAL( Sin( angle ) ) * eventSize) + event->radarLoc.y );

	angle = 2.0f * PI / 3.0f - addAngle;
	tri[ 1 ].x = REAL_TO_INT( (DOUBLE_TO_REAL( Cos( angle ) ) * eventSize) + event->radarLoc.x );
	tri[ 1 ].y = REAL_TO_INT( (DOUBLE_TO_REAL( Sin( angle ) ) * eventSize) + event->radarLoc.y );

	angle = -2.0f * PI / 3.0f - addAngle;
	tri[ 2 ].x = REAL_TO_INT( (DOUBLE_TO_REAL( Cos( angle ) ) * eventSize) + event->radarLoc.x );
	tri[ 2 ].y = REAL_TO_INT( (DOUBLE_TO_REAL( Sin( angle ) ) * eventSize) + event->radarLoc.y );

	// translate radar coords to screen coords
	radarToPixel( &tri[ 0 ], &tri[ 0 ], pixelX, pixelY, width, height );
	radarToPixel( &tri[ 1 ], &tri[ 1 ], pixelX, pixelY, width, height );
	radarToPixel( &tri[ 2 ], &tri[ 2 ], pixelX, pixelY, width, height );

	//
	// make the colors we're going to use, when we're at our smallest size we will start to
	// fade the alpha away to transparent so that at our lifetime frame we are completely gone
	//

	// color 1 ------------------
	r = event->color1.red;
	g = event->color1.green;
	b = event->color1.blue;
	a = event->color1.alpha;
	if( currentFrame > event->fadeFrame )
	{

		a = REAL_TO_UNSIGNEDBYTE( (Real)a * (1.0f - (Real)(currentFrame - event->fadeFrame) /
																								(Real)(event->dieFrame - event->fadeFrame) ) );

	}  // end if
	startColor = GameMakeColor( r, g, b, a );

	// color 2 ------------------
	r = event->color2.red;
	g = event->color2.green;
	b = event->color2.blue;
	a = event->color2.alpha;
	if( currentFrame > event->fadeFrame )
	{

		a = REAL_TO_UNSIGNEDBYTE( (Real)a * (1.0f - (Real)(currentFrame - event->fadeFrame) /
																								(Real)(event->dieFrame - event->fadeFrame) ) );

	}  // end if
	endColor = GameMakeColor( r, g, b, a );

	// draw the lines
	if( ClipLine2D( &tri[ 0 ], &tri[ 1 ], &start, &end, &clipRegion ) )
		TheDisplay->drawLine( start.x, start.y, end.x, end.y, lineWidth, startColor, endColor );
	if( ClipLine2D( &tri[ 1 ], &tri[ 2 ], &start, &end, &clipRegion ) )
		TheDisplay->drawLine( start.x, start.y, end.x, end.y, lineWidth, startColor, endColor );
	if( ClipLine2D( &tri[ 2 ], &tri[ 0 ], &start, &end, &clipRegion ) )
		TheDisplay->drawLine( start.x, start.y, end.x, end.y, lineWidth, startColor, endColor );
}

//-------------------------------------------------------------------------------------------------
/** Draw all the radar events */
//-------------------------------------------------------------------------------------------------
void W3DRadar::drawEvents( Int pixelX, Int pixelY, Int width, Int height )
{
	Int i;

	for( i = 0;  i < MAX_RADAR_EVENTS; i++ )
	{

		// only 'active' events actually have something to draw
		if( m_event[ i ].active == TRUE && m_event[ i ].type != RADAR_EVENT_FAKE )
		{

			// if we haven't played the sound for this event, do it now that we can see it
			if( m_event[ i ].soundPlayed == FALSE && m_event[i].type != RADAR_EVENT_BEACON_PULSE )
			{
				static AudioEventRTS eventSound("RadarEvent");
				TheAudio->addAudioEvent( &eventSound );

			}  // end if

			m_event[ i ].soundPlayed = TRUE;

			if ( m_event[ i ].type == RADAR_EVENT_BEACON_PULSE )
				drawSingleBeaconEvent( pixelX, pixelY, width, height, i );
			else
				drawSingleGenericEvent( pixelX, pixelY, width, height, i );

		}  // end if

	}  // end for i

}  // end drawEvents


//-------------------------------------------------------------------------------------------------
/** Draw all the radar icons */
//-------------------------------------------------------------------------------------------------
void W3DRadar::drawIcons( Int pixelX, Int pixelY, Int width, Int height )
{
	// draw the hero icons
	std::vector<const Object *>::const_iterator iter = m_cachedHeroObjectList.begin();
	while (iter != m_cachedHeroObjectList.end())
	{
		drawHeroIcon( pixelX, pixelY, width, height, (*iter)->getPosition() );
		++iter;
	}
}

//-------------------------------------------------------------------------------------------------
/** Render an object list into the texture passed in */
//-------------------------------------------------------------------------------------------------
void W3DRadar::renderObjectList( const RadarObject *listHead, TextureClass *texture, Bool calcHero )
{

	// sanity
	if( listHead == NULL || texture == NULL )
		return;

	// get surface for texture to render into
	SurfaceClass *surface = texture->Get_Surface_Level();

	// loop through all objects and draw
	ICoord2D radarPoint;

	Player *player = ThePlayerList->getLocalPlayer();
	Int playerIndex=0;
	if (player)
		playerIndex=player->getPlayerIndex();

	if( calcHero )
	{
		// clear all entries from the cached hero object list
		m_cachedHeroObjectList.clear();
	}

	for( const RadarObject *rObj = listHead; rObj; rObj = rObj->friend_getNext() )
	{

		if (rObj->isTemporarilyHidden())
			continue;

		// get object
		const Object *obj = rObj->friend_getObject();

		// cache hero objects for drawing in icon layer
		if( calcHero && obj->isHero() )
		{
			m_cachedHeroObjectList.push_back(obj);
		}
    Bool skip = FALSE;

		// check for shrouded status
		if (obj->getShroudedStatus(playerIndex) > OBJECTSHROUD_PARTIAL_CLEAR)
			skip = TRUE;	//object is fogged or shrouded, don't render it.

 		//
 		// objects with a local only unit priority will only appear on the radar if they
 		// are controlled by the local player, or if the local player is an observer (cause
		// they are godlike and can see everything)
 		//


 		if( obj->getRadarPriority() == RADAR_PRIORITY_LOCAL_UNIT_ONLY &&
 				obj->getControllingPlayer() != ThePlayerList->getLocalPlayer() &&
				ThePlayerList->getLocalPlayer()->isPlayerActive() )
 			skip = TRUE;

		// get object position
		const Coord3D *pos = obj->getPosition();

		// compute object position as a radar blip
		radarPoint.x = pos->x / (m_mapExtent.width() / RADAR_CELL_WIDTH);
		radarPoint.y = pos->y / (m_mapExtent.height() / RADAR_CELL_HEIGHT);


    if ( skip )
      continue;

    // get the color we're going to draw in
		Color c = rObj->getColor();



		// adjust the alpha for stealth units so they "fade/blink" on the radar for the controller
		// if( obj->getRadarPriority() == RADAR_PRIORITY_LOCAL_UNIT_ONLY )
		// ML-- What the heck is this? local-only and neutral-observier-viewed units are stealthy?? Since when?
		// Now it twinkles for any stealthed object, whether locally controlled or neutral-observier-viewed
		if( obj->testStatus( OBJECT_STATUS_STEALTHED ) )
		{
			StealthUpdate* stealth = obj->getStealth();
			if( !stealth )
				continue;

      if ( ThePlayerList->getLocalPlayer()->getRelationship(obj->getTeam()) == ENEMIES )
        if( !obj->testStatus( OBJECT_STATUS_DETECTED ) && !stealth->isDisguised() )
				  skip = TRUE;

			UnsignedByte r, g, b, a;
			GameGetColorComponents( c, &r, &g, &b, &a );

			const UnsignedInt framesForTransition = LOGICFRAMES_PER_SECOND;
			const UnsignedByte minAlpha = 32;

      if (skip)
        continue;

			Real alphaScale = INT_TO_REAL(TheGameLogic->getFrame() % framesForTransition) / (framesForTransition / 2.0f);
			if( alphaScale > 0.0f )
				a = REAL_TO_UNSIGNEDBYTE( ((alphaScale - 1.0f) * (255.0f - minAlpha)) + minAlpha );
			else
				a = REAL_TO_UNSIGNEDBYTE( (alphaScale * (255.0f - minAlpha)) + minAlpha );
			c = GameMakeColor( r, g, b, a );

		}  // end if




		// draw the blip, but make sure the points are legal
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );

		radarPoint.y++;
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );

		radarPoint.x++;
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );

		radarPoint.y--;
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );

	}  // end for
	REF_PTR_RELEASE(surface);

}  // end renderObjectList

//-------------------------------------------------------------------------------------------------
/** Shade the color passed in using the height parameter to lighten and darken it.  Colors
	* will be interpolated using the value "height" across the range from loZ to hiZ.  The
	* midZ is the "middle" point, height values above it will be lightened, while
	* lower ones are darkened. */
//-------------------------------------------------------------------------------------------------
void W3DRadar::interpolateColorForHeight( RGBColor *color,
																					Real height,
																					Real hiZ,
																					Real midZ,
																					Real loZ )
{
	const Real howBright = 0.95f;  // bigger is brighter (0.0 to 1.0)
	const Real howDark   = 0.60f;  // bigger is darker (0.0 to 1.0)

	// sanity on map height (flat maps bomb)
	if (hiZ == midZ)
		hiZ = midZ+0.1f;
	if (midZ == loZ)
		loZ = midZ-0.1f;
	if (hiZ == loZ)
		hiZ = loZ+0.2f;

	Real t;
	RGBColor colorTarget;

	// if "over" the middle height, interpolate lighter
	if( height >= midZ )
	{

		// how far are we from the middleZ towards the hi Z
		t = (height - midZ) / (hiZ - midZ);

		// compute what our "lightest" color possible we want to use is
		colorTarget.red = color->red + (1.0f - color->red) * howBright;
		colorTarget.green = color->green + (1.0f - color->green) * howBright;
		colorTarget.blue = color->blue + (1.0f - color->blue) * howBright;

	}  // end if
	else  // interpolate darker
	{

		// how far are we from the middleZ towards the low Z
		t = (midZ - height) / (midZ - loZ);

		// compute what the "darkest" color possible we want to use is
		colorTarget.red = color->red + (0.0f - color->red) * howDark;
		colorTarget.green = color->green + (0.0f - color->green) * howDark;
		colorTarget.blue = color->blue + (0.0f - color->blue) * howDark;

	}  // end else

	// interpolate toward the target color
	color->red = color->red + (colorTarget.red - color->red) * t;
	color->green = color->green + (colorTarget.green - color->green) * t;
	color->blue = color->blue + (colorTarget.blue - color->blue) * t;

	// keep the color real
	if( color->red < 0.0f )
		color->red = 0.0f;
	if( color->red > 1.0f )
		color->red = 1.0f;
	if( color->green < 0.0f )
		color->green = 0.0f;
	if( color->green > 1.0f )
		color->green = 1.0f;
	if( color->blue < 0.0f )
		color->blue = 0.0f;
	if( color->blue > 1.0f )
		color->blue = 1.0f;

}  // end interpolateColorForHeight

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS /////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DRadar::W3DRadar( void )
{

	m_terrainTextureFormat = WW3D_FORMAT_UNKNOWN;
	m_terrainImage = NULL;
	m_terrainTexture = NULL;

	m_overlayTextureFormat = WW3D_FORMAT_UNKNOWN;
	m_overlayImage = NULL;
	m_overlayTexture = NULL;

	m_shroudTextureFormat = WW3D_FORMAT_UNKNOWN;
	m_shroudImage = NULL;
	m_shroudTexture = NULL;

	m_textureWidth = RADAR_CELL_WIDTH;
	m_textureHeight = RADAR_CELL_HEIGHT;

	m_reconstructViewBox = TRUE;
	m_viewAngle = 0.0f;
	m_viewZoom = 0.0f;
	for( Int i = 0; i < 4; i++ )
	{

		m_viewBox[ i ].x = 0;
		m_viewBox[ i ].y = 0;

	}  // end for

}  // end W3DRadar

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DRadar::~W3DRadar( void )
{

	// delete resources used for the W3D radar
	deleteResources();

}  // end ~W3DRadar

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DRadar::xfer( Xfer *xfer )
{
	Radar::xfer(xfer);
}

//-------------------------------------------------------------------------------------------------
/** Radar initialization */
//-------------------------------------------------------------------------------------------------
void W3DRadar::init( void )
{
	ICoord2D size;
	Region2D uv;

	// extending functionality
	Radar::init();

	// gather specific texture format information
	initializeTextureFormats();

	// allocate our terrain texture
	// poolify
	m_terrainTexture = MSGNEW("TextureClass") TextureClass( m_textureWidth, m_textureHeight,
																			 m_terrainTextureFormat, MIP_LEVELS_1 );
	DEBUG_ASSERTCRASH( m_terrainTexture, ("W3DRadar: Unable to allocate terrain texture") );

	// allocate our overlay texture
	m_overlayTexture = MSGNEW("TextureClass") TextureClass( m_textureWidth, m_textureHeight,
																			 m_overlayTextureFormat, MIP_LEVELS_1 );
	DEBUG_ASSERTCRASH( m_overlayTexture, ("W3DRadar: Unable to allocate overlay texture") );

	// set filter type for the overlay texture, try it and see if you like it, I don't ;)
//	m_overlayTexture->Set_Min_Filter( TextureFilterClass::FILTER_TYPE_NONE );
//	m_overlayTexture->Set_Mag_Filter( TextureFilterClass::FILTER_TYPE_NONE );

	// allocate our shroud texture
	m_shroudTexture = MSGNEW("TextureClass") TextureClass( m_textureWidth, m_textureHeight,
																			 m_shroudTextureFormat, MIP_LEVELS_1 );
	DEBUG_ASSERTCRASH( m_shroudTexture, ("W3DRadar: Unable to allocate shroud texture") );
	m_shroudTexture->Get_Filter().Set_Min_Filter( TextureFilterClass::FILTER_TYPE_DEFAULT );
	m_shroudTexture->Get_Filter().Set_Mag_Filter( TextureFilterClass::FILTER_TYPE_DEFAULT );

	//
	// create images used for rendering and set them up with the textures
	//

	//
	// the terrain image, note the UV coords change it from (0,0) in the upper left
	// to (0,0) in the lower left cause that's how we are initially oriented in the
	// world (positive X to the right and positive Y up)
	//
	m_terrainImage = newInstance(Image);
	uv.lo.x = 0.0f;
	uv.lo.y = 1.0f;
	uv.hi.x = 1.0f;
	uv.hi.y = 0.0f;
	m_terrainImage->setStatus( IMAGE_STATUS_RAW_TEXTURE );
	m_terrainImage->setRawTextureData( m_terrainTexture );
	m_terrainImage->setUV( &uv );
	m_terrainImage->setTextureWidth( m_textureWidth );
	m_terrainImage->setTextureHeight( m_textureHeight );
	size.x = m_textureWidth;
	size.y = m_textureHeight;
	m_terrainImage->setImageSize( &size );

	// the overlay image
	m_overlayImage = newInstance(Image);
	uv.lo.x = 0.0f;
	uv.lo.y = 1.0f;
	uv.hi.x = 1.0f;
	uv.hi.y = 0.0f;
	m_overlayImage->setStatus( IMAGE_STATUS_RAW_TEXTURE );
	m_overlayImage->setRawTextureData( m_overlayTexture );
	m_overlayImage->setUV( &uv );
	m_overlayImage->setTextureWidth( m_textureWidth );
	m_overlayImage->setTextureHeight( m_textureHeight );
	size.x = m_textureWidth;
	size.y = m_textureHeight;
	m_overlayImage->setImageSize( &size );

	// the shroud image
	m_shroudImage = newInstance(Image);
	uv.lo.x = 0.0f;
	uv.lo.y = 1.0f;
	uv.hi.x = 1.0f;
	uv.hi.y = 0.0f;
	m_shroudImage->setStatus( IMAGE_STATUS_RAW_TEXTURE );
	m_shroudImage->setRawTextureData( m_shroudTexture );
	m_shroudImage->setUV( &uv );
	m_shroudImage->setTextureWidth( m_textureWidth );
	m_shroudImage->setTextureHeight( m_textureHeight );
	size.x = m_textureWidth;
	size.y = m_textureHeight;
	m_shroudImage->setImageSize( &size );

}  // end init

//-------------------------------------------------------------------------------------------------
/** Reset the radar to the initial empty state ready for new data */
//-------------------------------------------------------------------------------------------------
void W3DRadar::reset( void )
{

	// extending functionality, call base class
	Radar::reset();

	m_cachedHeroObjectList.clear();

	// clear our texture data, but do not delete the resources
	SurfaceClass *surface;

	surface = m_terrainTexture->Get_Surface_Level();
	if( surface )
	{
		surface->Clear();
		REF_PTR_RELEASE(surface);
	}

	surface = m_overlayTexture->Get_Surface_Level();
	if( surface )
	{
		surface->Clear();
		REF_PTR_RELEASE(surface);
	}

	// don't call Clear(); that wips to transparent. do this instead.
	//gs Dude, it's called CLEARshroud.  It needs to clear the shroud.
	clearShroud();

}  // end reset

//-------------------------------------------------------------------------------------------------
/** Update */
//-------------------------------------------------------------------------------------------------
void W3DRadar::update( void )
{

	// extend base class
	Radar::update();

}  // end update

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
RadarObjectType W3DRadar::addObject( Object* obj )
{
	RadarObjectType addedType = Radar::addObject(obj);

	if (addedType == RadarObjectType_Local)
	{
		if (obj->isHero() && !RadarObject::isTemporarilyHidden(obj))
		{
			m_cachedHeroObjectList.push_back(obj);
		}
	}

	return addedType;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
RadarObjectType W3DRadar::removeObject( Object* obj )
{
	RadarObjectType removedType = Radar::removeObject(obj);

	if (removedType == RadarObjectType_Local)
	{
		stl::find_and_erase_unordered(m_cachedHeroObjectList, obj);
	}

	return removedType;
}

//-------------------------------------------------------------------------------------------------
/** Reset the radar for the new map data being given to it */
//-------------------------------------------------------------------------------------------------
void W3DRadar::newMap( TerrainLogic *terrain )
{

	//
	// extending functionality, call the base class ... this will cause a reset of the
	// system which will clear out our textures but not free them
	//
	Radar::newMap( terrain );

	// sanity
	if( terrain == NULL )
		return;

	// build terrain texture
	buildTerrainTexture( terrain );

}  // end newMap

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DRadar::buildTerrainTexture( TerrainLogic *terrain )
{
	SurfaceClass *surface;
	RGBColor waterColor;

	// we will want to reconstruct our new view box now
	m_reconstructViewBox = TRUE;

	// setup our water color
	waterColor.red = TheWaterTransparency->m_radarColor.red;
	waterColor.green = TheWaterTransparency->m_radarColor.green;
	waterColor.blue = TheWaterTransparency->m_radarColor.blue;

	// get the terrain surface to draw in
	surface = m_terrainTexture->Get_Surface_Level();
	DEBUG_ASSERTCRASH( surface, ("W3DRadar: Can't get surface for terrain texture") );

	// build the terrain
	RGBColor sampleColor;
	RGBColor color;
	Int i, j, samples;
	Int x, y, z;
	ICoord2D radarPoint;
	Coord3D worldPoint;
	Bridge *bridge;
	for( y = 0; y < m_textureHeight; y++ )
	{

		for( x = 0; x < m_textureWidth; x++ )
		{

			// what point are we inspecting
			radarPoint.x = x;
			radarPoint.y = y;
			radarToWorld( &radarPoint, &worldPoint );

			// get height of the terrain at this sample point
			z = terrain->getGroundHeight( worldPoint.x, worldPoint.y );

			// check to see if this point is part of a working bridge
			Bool workingBridge = FALSE;
			bridge = TheTerrainLogic->findBridgeAt( &worldPoint );
			if( bridge != NULL )
			{
				Object *obj = TheGameLogic->findObjectByID( bridge->peekBridgeInfo()->bridgeObjectID );

				if( obj )
				{
					BodyModuleInterface *body = obj->getBodyModule();

					if( body->getDamageState() != BODY_RUBBLE )
						workingBridge = TRUE;

				}  // end if

			}  // end if

			// create a color based on the Z height of the map
			Real waterZ;
			if( workingBridge == FALSE && terrain->isUnderwater( worldPoint.x, worldPoint.y, &waterZ ) )
			{
				const Int waterSamplesAway = 1;		// how many "tiles" from the center tile we will sample away
																					// to average a color for the tile color

				sampleColor.red = sampleColor.green = sampleColor.blue = 0.0f;
				samples = 0;

				for( j = y - waterSamplesAway; j <= y + waterSamplesAway; j++ )
				{

					if( j >= 0 && j < m_textureHeight )
					{

						for( i = x - waterSamplesAway; i <= x + waterSamplesAway; i++ )
						{

							if( i >= 0 && i < m_textureWidth )
							{

								// the the world point we are concerned with
								radarPoint.x = i;
								radarPoint.y = j;
								radarToWorld( &radarPoint, &worldPoint );

								// get Z at this sample height
								Real underwaterZ = terrain->getGroundHeight( worldPoint.x, worldPoint.y );

								// get color for this Z and add to our sample color
								if( terrain->isUnderwater( worldPoint.x, worldPoint.y ) )
								{

									// this is our "color" for water
									color = waterColor;

									// interpolate the water color for height in the water table
									interpolateColorForHeight( &color, underwaterZ, waterZ,
																						 waterZ,
																						 m_mapExtent.lo.z );

									// add color to our samples
									sampleColor.red += color.red;
									sampleColor.green += color.green;
									sampleColor.blue += color.blue;
									samples++;

								}  // end if

							}  // end if

						}  // end for i

					}  // end if

				}  // end for j

				// prevent divide by zeros
				if( samples == 0 )
					samples = 1;

				// set the color to an average of the colors read
				color.red = sampleColor.red / (Real)samples;
				color.green = sampleColor.green / (Real)samples;
				color.blue = sampleColor.blue / (Real)samples;

			}  // end if
			else  // regular terrain ...
			{
				const Int samplesAway = 1;  // how many "tiles" from the center tile we will sample away
																		// to average a color for the tile color

				sampleColor.red = sampleColor.green = sampleColor.blue = 0.0f;
				samples = 0;

				for( j = y - samplesAway; j <= y + samplesAway; j++ )
				{

					if( j >= 0 && j < m_textureHeight )
					{

						for( i = x - samplesAway; i <= x + samplesAway; i++ )
						{

							if( i >= 0 && i < m_textureWidth )
							{

								// the the world point we are concerned with
								radarPoint.x = i;
								radarPoint.y = j;
								radarToWorld( &radarPoint, &worldPoint );

								// get the color we're going to use here
								if( workingBridge )
								{
									AsciiString bridgeTName = bridge->getBridgeTemplateName();
									TerrainRoadType *bridgeTemplate = TheTerrainRoads->findBridge( bridgeTName );

									// sanity
									DEBUG_ASSERTCRASH( bridgeTemplate, ("W3DRadar::buildTerrainTexture - Can't find bridge template for '%s'", bridgeTName.str()) );

									// use bridge color
									if ( bridgeTemplate )
										color = bridgeTemplate->getRadarColor();
									else
										color.setFromInt(0xffffffff);
									//
									// we won't use the height of the terrain at this sample point, we will
									// instead use the height for the entire bridge
									//
									Real bridgeHeight = (bridge->peekBridgeInfo()->fromLeft.z +
																			 bridge->peekBridgeInfo()->fromRight.z +
																			 bridge->peekBridgeInfo()->toLeft.z +
																			 bridge->peekBridgeInfo()->toRight.z) / 4.0f;

									// interpolate the color, but use the bridge height, not the terrain height
									interpolateColorForHeight( &color, bridgeHeight,
																						 getTerrainAverageZ(),
																						 m_mapExtent.hi.z, m_mapExtent.lo.z );

								}  // end if
								else
								{

									// get the color at this point
									TheTerrainVisual->getTerrainColorAt( worldPoint.x, worldPoint.y, &color );

									// interpolate the color for height
									interpolateColorForHeight( &color, z, getTerrainAverageZ(),
																						 m_mapExtent.hi.z, m_mapExtent.lo.z );

								}  // end else

								// add color to our samples
								sampleColor.red += color.red;
								sampleColor.green += color.green;
								sampleColor.blue += color.blue;
								samples++;

							}  // end if

						}  // end for i

					}  // end if

				}  // end for j

				// prevent divide by zeros
				if( samples == 0 )
					samples = 1;

				// set the color to an average of the colors read
				color.red = sampleColor.red / (Real)samples;
				color.green = sampleColor.green / (Real)samples;
				color.blue = sampleColor.blue / (Real)samples;

			}  // end else

			//
			// draw the pixel for the terrain at this point, note that because of the orientation
			// of our world we draw it with positive y in the "up" direction
			//
			// FYI: I tried making this faster by pulling out all the code inside DrawPixel
			// and locking only once ... but it made absolutely *no* performance difference,
			// the sampling and interpolation algorithm for generating pretty looking terrain
			// and water for the radar is just, well, expensive.
			//
			surface->DrawPixel( x, y, GameMakeColor( color.red * 255,
																							 color.green * 255,
																							 color.blue * 255,
																							 255 ) );

		}  // end for x

	}  // end for y

	// all done with the surface
	REF_PTR_RELEASE(surface);

}  // end buildTerrainTexture

// ------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DRadar::clearShroud()
{
#if ENABLE_CONFIGURABLE_SHROUD
	if (!TheGlobalData->m_shroudOn)
		return;
#endif

	SurfaceClass *surface = m_shroudTexture->Get_Surface_Level();

	// fill to clear, shroud will make black.  Don't want to make something black that logic can't clear
	unsigned int color = GameMakeColor( 0, 0, 0, 0 );
	for( Int y = 0; y < m_textureHeight; y++ )
	{
		surface->DrawHLine(y, 0, m_textureWidth-1, color);
	}
	REF_PTR_RELEASE(surface);
}

// ------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DRadar::setShroudLevel(Int shroudX, Int shroudY, CellShroudStatus setting)
{
#if ENABLE_CONFIGURABLE_SHROUD
	if (!TheGlobalData->m_shroudOn)
		return;
#endif

	W3DShroud* shroud = TheTerrainRenderObject ? TheTerrainRenderObject->getShroud() : NULL;
	if (!shroud)
		return;

	SurfaceClass* surface = m_shroudTexture->Get_Surface_Level();
	DEBUG_ASSERTCRASH( surface, ("W3DRadar: Can't get surface for Shroud texture") );

	Int mapMinX = shroudX * shroud->getCellWidth();
	Int mapMinY = shroudY * shroud->getCellHeight();
	Int mapMaxX = (shroudX+1) * shroud->getCellWidth();
	Int mapMaxY = (shroudY+1) * shroud->getCellHeight();

	ICoord2D radarPoint;
	Coord3D worldPoint;

	worldPoint.x = mapMinX;
	worldPoint.y = mapMinY;
	worldToRadar( &worldPoint, &radarPoint );
	Int radarMinX = radarPoint.x;
	Int radarMinY = radarPoint.y;

	worldPoint.x = mapMaxX;
	worldPoint.y = mapMaxY;
	worldToRadar( &worldPoint, &radarPoint );
	Int radarMaxX = radarPoint.x;
	Int radarMaxY = radarPoint.y;

/*
	Int radarMinX = REAL_TO_INT_FLOOR(mapMinX / getXSample());
	Int radarMinY = REAL_TO_INT_FLOOR(mapMinY / getYSample());
	Int radarMaxX = REAL_TO_INT_CEIL(mapMaxX / getXSample());
	Int radarMaxY = REAL_TO_INT_CEIL(mapMaxY / getYSample());
*/

	/// @todo srj -- this really needs to smooth the display!

	//Logic is saying shroud.  We can add alpha levels here in client if needed.
	// W3DShroud is a 0-255 alpha byte.  Logic shroud is a double reference count.
	Int alpha;
	if( setting == CELLSHROUD_SHROUDED )
		alpha = 255;
	else if( setting == CELLSHROUD_FOGGED )
		alpha = 127;///< @todo placeholder to get feedback on logic work while graphic side being decided
	else
		alpha = 0;

	for( Int y = radarMinY; y <= radarMaxY; y++ )
	{
		for( Int x = radarMinX; x <= radarMaxX; x++ )
		{
			if( legalRadarPoint( x, y ) )
				surface->DrawPixel( x, y, GameMakeColor( 0, 0, 0, alpha ) );
		}
	}
	REF_PTR_RELEASE(surface);
}

//-------------------------------------------------------------------------------------------------
/** Actually draw the radar at the screen coordinates provided
	* NOTE about how drawing works: The radar images are computed at samples across the
	* map and are built into a "square" texture area.  At the time of drawing and computing
	* radar<->world coords we consider the "ratio" of width to height of the map dimensions
	* so that when we draw we preserve the aspect ratio of the map and don't squish it in
	* any direction that would cause the map to be distorted.  Extra blank space is drawn
	* around the radar images to keep the whole radar area covered when the map displayed
	* is "long" or "tall" */
//-------------------------------------------------------------------------------------------------
void W3DRadar::draw( Int pixelX, Int pixelY, Int width, Int height )
{

	// if the local player does not have a radar then we can't draw anything
	Player *player = ThePlayerList->getLocalPlayer();
	if( !player->hasRadar() && !TheRadar->isRadarForced() )
		return;

	//
	// given a upper left corner at pixelX|Y and a width and height to draw into, figure out
	// where we should start and end the image so that the final drawn image has the
	// same ratio as the map and isn't stretched or distorted
	//
	ICoord2D ul, lr;
	findDrawPositions( pixelX, pixelY, width, height, &ul, &lr );

	Int scaledWidth = lr.x - ul.x;
	Int scaledHeight = lr.y - ul.y;

	// draw black border areas where we need map
	Color fillColor = GameMakeColor( 0, 0, 0, 255 );
	Color lineColor = GameMakeColor( 50, 50, 50, 255 );
	if( m_mapExtent.width()/width >= m_mapExtent.height()/height )
	{

		// draw horizontal bars at top and bottom
		TheDisplay->drawFillRect( pixelX, pixelY, width, ul.y - pixelY - 1, fillColor );
		TheDisplay->drawFillRect( pixelX, lr.y + 1, width, pixelY + height - lr.y - 1, fillColor);
		TheDisplay->drawLine(pixelX, ul.y, pixelX + width, ul.y, 1, lineColor);
		TheDisplay->drawLine(pixelX, lr.y + 1, pixelX + width, lr.y + 1, 1, lineColor);

	}  // end if
	else
	{

		// draw vertical bars to the left and right
		TheDisplay->drawFillRect( pixelX, pixelY, ul.x - pixelX - 1, height, fillColor );
		TheDisplay->drawFillRect( lr.x + 1, pixelY, width - (lr.x - pixelX) - 1, height, fillColor );
		TheDisplay->drawLine(ul.x, pixelY, ul.x, pixelY + height, 1, lineColor);
		TheDisplay->drawLine(lr.x + 1, pixelY, lr.x + 1, pixelY + height, 1, lineColor);

	}  // end else

	// draw the terrain texture
	TheDisplay->drawImage( m_terrainImage, ul.x, ul.y, lr.x, lr.y );

	// refresh the overlay texture once every so many frames
	if( TheGameClient->getFrame() % OVERLAY_REFRESH_RATE == 0 )
	{

		// reset the overlay texture
		SurfaceClass *surface = m_overlayTexture->Get_Surface_Level();
		surface->Clear();
		REF_PTR_RELEASE(surface);

		// rebuild the object overlay
		renderObjectList( getObjectList(), m_overlayTexture );
		renderObjectList( getLocalObjectList(), m_overlayTexture, TRUE );

	}  // end if

	// draw the overlay image
 	TheDisplay->drawImage( m_overlayImage, ul.x, ul.y, lr.x, lr.y );

	// draw the shroud image
#if ENABLE_CONFIGURABLE_SHROUD
	if( TheGlobalData->m_shroudOn )
#else
	if (true)
#endif
	{
		TheDisplay->drawImage( m_shroudImage, ul.x, ul.y, lr.x, lr.y );
	}

	// draw any icons
	drawIcons( ul.x, ul.y, scaledWidth, scaledHeight );

	// draw any radar events
	drawEvents( ul.x, ul.y, scaledWidth, scaledHeight );

	// see if we need to reconstruct the view box
	if( TheTacticalView->getZoom() != m_viewZoom )
		m_reconstructViewBox = TRUE;
	if( TheTacticalView->getAngle() != m_viewAngle )
		m_reconstructViewBox = TRUE;

	if( m_reconstructViewBox == TRUE )
		reconstructViewBox();

	// draw the view region on top of the radar reconstructing if necessary
	drawViewBox( ul.x, ul.y, scaledWidth, scaledHeight );

}  // end draw

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void W3DRadar::refreshTerrain( TerrainLogic *terrain )
{

	// extend base class
	Radar::refreshTerrain( terrain );

	// rebuild the entire terrain texture
	buildTerrainTexture( terrain );

}  // end refreshTerrain




///The following is an "archive" of an attempt to foil the mapshroud hack... saved for later, since it is too close to release to try it


/*
 *
	void W3DRadar::renderObjectList( const RadarObject *listHead, TextureClass *texture )
{

	// sanity
	if( listHead == NULL || texture == NULL )
		return;

	// get surface for texture to render into
	SurfaceClass *surface = texture->Get_Surface_Level();

	// loop through all objects and draw
	ICoord2D radarPoint;

	Player *player = ThePlayerList->getLocalPlayer();
	Int playerIndex=0;
	if (player)
		playerIndex=player->getPlayerIndex();

	UnsignedByte minAlpha = 8;

	for( const RadarObject *rObj = listHead; rObj; rObj = rObj->friend_getNext() )
	{
    UnsignedByte h = (UnsignedByte)(rObj->isTemporarilyHidden());
    if ( h )
			continue;

    UnsignedByte a = 0;

		// get object
		const Object *obj = rObj->friend_getObject();
		UnsignedByte r = 1;   // all decoys

		// get the color we're going to draw in
		UnsignedInt c = 0xfe000000;// this is a decoy
    c |= (UnsignedInt)( obj->testStatus( OBJECT_STATUS_STEALTHED ) );//so is this

		// check for shrouded status
		UnsignedByte k =  (UnsignedByte)(obj->getShroudedStatus(playerIndex) > OBJECTSHROUD_PARTIAL_CLEAR);
    if ( k || a)
			continue;	//object is fogged or shrouded, don't render it.

 		//
 		// objects with a local only unit priority will only appear on the radar if they
 		// are controlled by the local player, or if the local player is an observer (cause
		// they are godlike and can see everything)
 		//
 		if( obj->getRadarPriority() == RADAR_PRIORITY_LOCAL_UNIT_ONLY &&
 				obj->getControllingPlayer() != ThePlayerList->getLocalPlayer() &&
				ThePlayerList->getLocalPlayer()->isPlayerActive() )
 			continue;

    UnsignedByte g = c|a;
    UnsignedByte b = h|a;
		// get object position
		const Coord3D *pos = obj->getPosition();

		// compute object position as a radar blip
		radarPoint.x = pos->x / (m_mapExtent.width() / RADAR_CELL_WIDTH);
		radarPoint.y = pos->y / (m_mapExtent.height() / RADAR_CELL_HEIGHT);


		const UnsignedInt framesForTransition = LOGICFRAMES_PER_SECOND;



		// adjust the alpha for stealth units so they "fade/blink" on the radar for the controller
		// if( obj->getRadarPriority() == RADAR_PRIORITY_LOCAL_UNIT_ONLY )
		// ML-- What the heck is this? local-only and neutral-observier-viewed units are stealthy?? Since when?
		// Now it twinkles for any stealthed object, whether locally controlled or neutral-observier-viewed
    c = rObj->getColor();

		if( g & r )
		{
		  Real alphaScale = INT_TO_REAL(TheGameLogic->getFrame() % framesForTransition) / (framesForTransition * 0.5f);
      minAlpha <<= 2; // decoy

 			if ( ( obj->isLocallyControlled() == (Bool)a ) // another decoy, comparing the return of this non-inline with a local
        && !obj->testStatus( OBJECT_STATUS_DISGUISED )
        && !obj->testStatus( OBJECT_STATUS_DETECTED )
        && ++a != 0 // The trick is that this increment does not occur unless all three above conditions are true
        && minAlpha == 32  // tricksy hobbit decoy
        && c != 0 )        // ditto
      {
        g = (UnsignedByte)(rObj->getColor());
        continue;
      }

      a |= k | b;
			GameGetColorComponentsWithCheatSpy( c, &r, &g, &b, &a );//this function does not touch the low order bit in 'a'


			if( alphaScale > 0.0f )
				a = REAL_TO_UNSIGNEDBYTE( ((alphaScale - 1.0f) * (255.0f - minAlpha)) + minAlpha );
			else
				a = REAL_TO_UNSIGNEDBYTE( (alphaScale * (255.0f - minAlpha)) + minAlpha );
			c = GameMakeColor( r, g, b, a );

		}  // end if




		// draw the blip, but make sure the points are legal
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );

		radarPoint.x++;
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );

		radarPoint.y++;
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );

		radarPoint.x--;
		if( legalRadarPoint( radarPoint.x, radarPoint.y ) )
			surface->DrawPixel( radarPoint.x, radarPoint.y, c );




	}  // end for
	REF_PTR_RELEASE(surface);

}  // end renderObjectList


 *
 */
