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

// ScorchTool.h
// Author: Dennis Griffin, April 2002

#pragma once

#ifndef SCORCHTOOL_H
#define SCORCHTOOL_H

#include "Tool.h"
class WorldHeightMapEdit;
class MapObject;
/*************************************************************************/
/**                             ScorchTool
***************************************************************************/
///  Scorch tool.
class ScorchTool : public Tool
{
public:
	ScorchTool(void);
	~ScorchTool(void);

protected:
	Coord3D m_mouseDownPt;

protected:
	MapObject *pickScorch(Coord3D loc);

public:
	/// Perform tool on mouse down.
	virtual void mouseDown(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc);
	virtual void mouseMoved(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc);
	virtual void mouseUp(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc);
	virtual void activate(); ///< Become the current tool.
	virtual void deactivate(); ///< Become not the current tool.
};


#endif //SCORCHTOOL_H
