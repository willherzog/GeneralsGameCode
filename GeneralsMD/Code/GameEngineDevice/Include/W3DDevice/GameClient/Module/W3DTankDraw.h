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

// FIEL: W3DTankDraw.h ////////////////////////////////////////////////////////////////////////////
// Draw a tank
// Author: Michael S. Booth, October 2001
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _W3D_TANK_DRAW_H_
#define _W3D_TANK_DRAW_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/DrawModule.h"
#include "GameClient/ParticleSys.h"
#include "W3DDevice/GameClient/Module/W3DModelDraw.h"
#include "WW3D2/hanim.h"
#include "WW3D2/rendobj.h"
#include "WW3D2/part_emt.h"

//-------------------------------------------------------------------------------------------------
class W3DTankDrawModuleData : public W3DModelDrawModuleData
{
public:
	AsciiString m_treadDebrisNameLeft;
	AsciiString m_treadDebrisNameRight;

	Real m_treadAnimationRate;	///<amount of tread texture to scroll per sec.  1.0 == full width.
	Real m_treadPivotSpeedFraction;	///<fraction of locomotor speed below which we allow pivoting.
	Real m_treadDriveSpeedFraction;	///<fraction of locomotor speed below which treads stop animating.

	W3DTankDrawModuleData();
	~W3DTankDrawModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
class W3DTankDraw : public W3DModelDraw
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DTankDraw, "W3DTankDraw" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( W3DTankDraw, W3DTankDrawModuleData )

public:

	W3DTankDraw( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void setHidden(Bool h);
	virtual void doDrawModule(const Matrix3D* transformMtx);
	virtual void setFullyObscuredByShroud(Bool fullyObscured);

protected:
	virtual void onRenderObjRecreated(void);

protected:

	/// debris emitters for when tank is moving
	ParticleSystem* m_treadDebrisLeft;
	ParticleSystem* m_treadDebrisRight;

	RenderObjClass *m_prevRenderObj;

	enum TreadType { TREAD_LEFT, TREAD_RIGHT, TREAD_MIDDLE };	//types of treads for different vehicles
	enum {MAX_TREADS_PER_TANK=4};

	struct TreadObjectInfo
	{
		RenderObjClass	*m_robj;	///<sub-object for tread
		TreadType	m_type;			///<kind of tread
		RenderObjClass::Material_Override m_materialSettings;	///<used to set current uv scroll amount.
	};

	TreadObjectInfo m_treads[MAX_TREADS_PER_TANK];
	Int m_treadCount;
	Coord3D m_lastDirection;		///< orientation of tank last time it was drawn.

	void createEmitters( void );					///< Create particle effects.
	void tossEmitters( void );					///< Create particle effects.

	void startMoveDebris( void );												///< start creating debris from the tank treads
	void stopMoveDebris( void );												///< stop creating debris from the tank treads
	void updateTreadObjects(void);												///< update pointers to sub-objects like treads.
	void updateTreadPositions(Real uvDelta);									///< update uv coordinates on each tread
};

#endif // _W3D_TANK_DRAW_H_

