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

// FILE: W3DPropDraw.h //////////////////////////////////////////////////////////////////////////
// Author: John Ahlquist June 2003
// Desc:   Simple prop drawing draw method.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __W3DPropDraw_H_
#define __W3DPropDraw_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/DrawModule.h"
#include "WW3D2/line3d.h"

//-------------------------------------------------------------------------------------------------
class W3DPropDrawModuleData : public ModuleData
{
public:
	AsciiString m_modelName;

	W3DPropDrawModuleData();
	~W3DPropDrawModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
	// ugh, hack
	virtual const W3DPropDrawModuleData* getAsW3DPropDrawModuleData() const { return this; }
};

//-------------------------------------------------------------------------------------------------
/** W3D prop draw */
//-------------------------------------------------------------------------------------------------
class W3DPropDraw : public DrawModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DPropDraw, "W3DPropDraw" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( W3DPropDraw, W3DPropDrawModuleData )

public:

	W3DPropDraw( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void doDrawModule(const Matrix3D* transformMtx);
	virtual void setShadowsEnabled(Bool enable) { }
	virtual void releaseShadows(void) {};	///< we don't care about preserving temporary shadows.
	virtual void allocateShadows(void) {};	///< we don't care about preserving temporary shadows.
	virtual void setFullyObscuredByShroud(Bool fullyObscured) { }
	virtual void reactToTransformChange(const Matrix3D* oldMtx, const Coord3D* oldPos, Real oldAngle);
	virtual void reactToGeometryChange() { }

protected:
	Bool m_propAdded;

};

#endif // __W3DPropDraw_H_

