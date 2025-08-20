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

// FILE: W3DGameClient.cpp /////////////////////////////////////////////////
//
// W3DImplementaion of the GameClient.  If there were a client/server
// architecture, this game interface could be thought of as the "client"
// that the user uses to interact with the logic of the game world which
// would be known as the "server"
//
// Author: Colin Day, April 2001
//
///////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
#include <stdlib.h>

// USER INCLUDES //////////////////////////////////////////////////////////////

#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/ModuleFactory.h"
#include "Common/RandomValue.h"
#include "Common/GlobalData.h"
#include "Common/GameLOD.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/ParticleSys.h"
#include "GameClient/RayEffect.h"
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "W3DDevice/GameClient/W3DGameClient.h"
#include "W3DDevice/GameClient/W3DStatusCircle.h"
#include "W3DDevice/GameClient/W3DScene.h"
#include "W3DDevice/GameClient/W3DShadow.h"
#include "W3DDevice/GameClient/HeightMap.h"
#include "WW3D2/part_emt.h"
#include "WW3D2/hanim.h"
#include "WW3D2/htree.h"
#include "WW3D2/animobj.h"  ///< @todo superhack for demo, remove!

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DGameClient::W3DGameClient()
{

}  // end W3DGameClient

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DGameClient::~W3DGameClient()
{

}  // end ~W3DGameClient

//-------------------------------------------------------------------------------------------------
/** Initialize resources for the w3d game client */
//-------------------------------------------------------------------------------------------------
void W3DGameClient::init( void )
{

	// extending initialization routine
	GameClient::init();

}  // end init

//-------------------------------------------------------------------------------------------------
/** Per frame udpate, note we are extending functionality */
//-------------------------------------------------------------------------------------------------
void W3DGameClient::update( void )
{

	// call base
	GameClient::update();

}  // end update

//-------------------------------------------------------------------------------------------------
/** Reset this device client system.  Note we are extending reset functionality from
	* the device independent client */
//-------------------------------------------------------------------------------------------------
void W3DGameClient::reset( void )
{

	// call base class
	GameClient::reset();

}  // end reset

//-------------------------------------------------------------------------------------------------
/** allocate a new drawable using the thing template for initialization.
	* if we want to have the thing manager actually contain the pools of
	* object and drawable storage it seems OK to have it be friends with the
	* GameLogic/Client for those purposes, or we could put the allocation pools
	* in the GameLogic and GameClient themselves */
//-------------------------------------------------------------------------------------------------
Drawable *W3DGameClient::friend_createDrawable( const ThingTemplate *tmplate,
																								DrawableStatus statusBits )
{
	Drawable *draw = NULL;

	// sanity
	if( tmplate == NULL )
		return NULL;

	draw = newInstance(Drawable)( tmplate, statusBits );

	return draw;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DGameClient::addScorch(const Coord3D *pos, Real radius, Scorches type)
{
	if (TheTerrainRenderObject)
	{
		Vector3 loc(pos->x, pos->y, pos->z);
		TheTerrainRenderObject->addScorch(loc, radius, type);
	}
}

//-------------------------------------------------------------------------------------------------
/** create an effect that requires a start and end location */
//-------------------------------------------------------------------------------------------------
void W3DGameClient::createRayEffectByTemplate( const Coord3D *start,
																		 const Coord3D *end,
																		 const ThingTemplate* tmpl )
{
	Drawable *draw = TheThingFactory->newDrawable(tmpl);

	if( draw )
	{
		Coord3D pos;

		// add to world, the location of the drawable is at the midpoint of laser
		pos.x = (end->x - start->x) * 0.5f + start->x;
		pos.y = (end->y - start->y) * 0.5f + start->y;
		pos.z = (end->z - start->z) * 0.5f + start->z;
		draw->setPosition( &pos );

		// add this ray effect to the list of ray effects
		TheRayEffects->addRayEffect( draw, start, end );

	}  // end if

}  // end createRayEffectByTemplate

//-------------------------------------------------------------------------------------------------
/**  Tell all the drawables what time of day it is now */
//-------------------------------------------------------------------------------------------------
void W3DGameClient::setTimeOfDay( TimeOfDay tod )
{

	GameClient::setTimeOfDay(tod);

	//tell cloud/water plane to update its lighting/texture
	if (TheWaterRenderObj)
		TheWaterRenderObj->setTimeOfDay(tod);
	if (TheW3DShadowManager)
		TheW3DShadowManager->setTimeOfDay(tod);

	//tell the display to update its lighting
	TheDisplay->setTimeOfDay( tod );

}  // end setTimeOfDay


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DGameClient::setTeamColor(Int red, Int green, Int blue)
{

	W3DStatusCircle::setColor(red, green, blue);

}  // end setTeamColor

//-------------------------------------------------------------------------------------------------
/** temporary entry point for adjusting LOD for development testing. */
//-------------------------------------------------------------------------------------------------
void W3DGameClient::adjustLOD( Int adj )
{
	if (TheGlobalData == NULL)
		return;

	TheWritableGlobalData->m_textureReductionFactor += adj;

	if (TheWritableGlobalData->m_textureReductionFactor > 4)
		TheWritableGlobalData->m_textureReductionFactor = 4;	//16x less resolution is probably enough.
	if (TheWritableGlobalData->m_textureReductionFactor < 0)
		TheWritableGlobalData->m_textureReductionFactor = 0;

	if (WW3D::Get_Texture_Reduction() != TheWritableGlobalData->m_textureReductionFactor)
	{	WW3D::Set_Texture_Reduction(TheWritableGlobalData->m_textureReductionFactor,6);
		TheGameLODManager->setCurrentTextureReduction(TheWritableGlobalData->m_textureReductionFactor);
	}

//I commented this out because we're no longer using terrain LOD.  So I
//stole this function and keys to adjust the texture resolution instead. -MW

//	if( TheTerrainRenderObject )
//		TheTerrainRenderObject->adjustTerrainLOD( adj );

}  // end adjustLOD
