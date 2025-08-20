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

// FILE: DefaultProductionExitUpdate.cpp /////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, January, 2002
// Desc:		Hand off produced Units to me so I can Exit them into the world with my specific style
//					This instance simply spits the guy out at a point.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "Lib/BaseType.h"
#include "GameLogic/AI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/DefaultProductionExitUpdate.h"
#include "GameLogic/Object.h"
//#include "GameLogic/PartitionManager.h"

//-------------------------------------------------------------------------------------------------
DefaultProductionExitUpdate::DefaultProductionExitUpdate( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	// no rally point has been set
	m_rallyPointExists = false;
	// Added By Sadullah Nader
	// Initialization missing and needed

	m_rallyPoint.zero();

	//

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

//-------------------------------------------------------------------------------------------------
DefaultProductionExitUpdate::~DefaultProductionExitUpdate()
{
}

//-------------------------------------------------------------------------------------------------
void DefaultProductionExitUpdate::exitObjectViaDoor( Object *newObj, ExitDoorType exitDoor )
{
	DEBUG_ASSERTCRASH(exitDoor == DOOR_1, ("multiple exit doors not supported here"));

	Object *creationObject = getObject();
	if (creationObject)
	{
		const DefaultProductionExitUpdateModuleData* md = getDefaultProductionExitUpdateModuleData();

		Real exitAngle = creationObject->getOrientation();
		const Matrix3D *transform = creationObject->getTransformMatrix();
		Vector3 loc;
		Coord3D createPoint;

		//
		// calculate the position to create the object at, we take the coord specified
		// in INI which is in model space, rotate it to match the building angle
		// and translate for building location via a transform call
		//
		loc.Set( md->m_unitCreatePoint.x, md->m_unitCreatePoint.y, md->m_unitCreatePoint.z );
		transform->Transform_Vector( *transform, loc, &loc );

		// make sure the point is on the terrain
		loc.Z = TheTerrainLogic ? TheTerrainLogic->getLayerHeight( loc.X, loc.Y, creationObject->getLayer() ) : 0.0f;

		// we need it in Coord3D form
		createPoint.x = loc.X;
		createPoint.y = loc.Y;
		createPoint.z = loc.Z;

		newObj->setPosition( &createPoint );
		newObj->setOrientation( exitAngle );
		newObj->setLayer( creationObject->getLayer() );

		/** @todo This really should be automatically wrapped up in an actication sequence
		for objects in general */
		// tell the AI about it
		TheAI->pathfinder()->addObjectToPathfindMap( newObj );
		Coord3D tmp;
		getNaturalRallyPoint(tmp);
		std::vector<Coord3D> exitPath;
		exitPath.push_back(tmp);

		AIUpdateInterface  *ai = newObj->getAIUpdateInterface();
		if (m_rallyPointExists)
		{
			tmp = m_rallyPoint;
			if (ai && ai->isDoingGroundMovement())
			{
				if (TheAI->pathfinder()->adjustDestination(newObj, ai->getLocomotorSet(), &tmp))
					exitPath.push_back(tmp);

			}
		}
		if (ai) {
			ai->aiFollowExitProductionPath( &exitPath, creationObject, CMD_FROM_AI );
		}
	}

}



Bool DefaultProductionExitUpdate::getExitPosition( Coord3D& exitPosition ) const
{
	const Object *obj = getObject();
	if (!obj)
		return FALSE;

	const Matrix3D *transform = obj->getTransformMatrix();

	const DefaultProductionExitUpdateModuleData *md = getDefaultProductionExitUpdateModuleData();

	Vector3 loc;
	loc.Set( md->m_unitCreatePoint.x, md->m_unitCreatePoint.y, md->m_unitCreatePoint.z );
	transform->Transform_Vector( *transform, loc, &loc );

	exitPosition.x = loc.X;
	exitPosition.y = loc.Y;
	exitPosition.z = loc.Z;

	return TRUE;

}

//-------------------------------------------------------------------------------------------------
Bool DefaultProductionExitUpdate::getNaturalRallyPoint( Coord3D& rallyPoint, Bool offset ) const
{
	const DefaultProductionExitUpdateModuleData *data = getDefaultProductionExitUpdateModuleData();
	Vector3 p;

	//
	// get the natural rally point from the INI definition, this coord is in model space relative
	// to the model (0,0,0)
	//
	p.X = data->m_naturalRallyPoint.x;
	p.Y = data->m_naturalRallyPoint.y;
	p.Z = data->m_naturalRallyPoint.z;

	if ( offset )
	{
		Vector3 offset = p;
		offset.Normalize();
		offset *= (2*PATHFIND_CELL_SIZE_F);
		p+=offset;
	}

	// transform the point into world space
	const Matrix3D *transform = getObject()->getTransformMatrix();
	transform->Transform_Vector( *transform, p, &p );

	rallyPoint.x = p.X; rallyPoint.y = p.Y; rallyPoint.z = p.Z;
	return TRUE;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DefaultProductionExitUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void DefaultProductionExitUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// rally point
	xfer->xferCoord3D( &m_rallyPoint );

	// rally point exists
	xfer->xferBool( &m_rallyPointExists );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DefaultProductionExitUpdate::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

}  // end loadPostProcess
