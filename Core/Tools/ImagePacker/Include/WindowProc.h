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

// FILE: WindowProc.h /////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Westwood Studios Pacific.
//
//                       Confidential Information
//                Copyright (C) 2001 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
// Project:    ImagePacker
//
// File name:  WindowProc.h
//
// Created:    Colin Day, August 2001
//
// Desc:       Dialog procedure header for image packer utility
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __WINDOWPROC_H_
#define __WINDOWPROC_H_

// SYSTEM INCLUDES ////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////

// FORWARD REFERENCES /////////////////////////////////////////////////////////

// TYPE DEFINES ///////////////////////////////////////////////////////////////

// INLINING ///////////////////////////////////////////////////////////////////

// EXTERNALS //////////////////////////////////////////////////////////////////
extern BOOL CALLBACK ImagePackerProc( HWND hWndDialog, UINT message,
																			WPARAM wParam, LPARAM lParam );

extern HWND MakePreviewDisplay( void );
extern void UpdatePreviewWindow( void );
extern LRESULT CALLBACK PreviewProc( HWND hWnd, UINT message,
																		 WPARAM wParam, LPARAM lParam );

extern BOOL CALLBACK ImageErrorProc( HWND hWndDialog, UINT message,
																		 WPARAM wParam, LPARAM lParam );

extern BOOL CALLBACK PageErrorProc( HWND hWndDialog, UINT message,
																		WPARAM wParam, LPARAM lParam );

extern BOOL CALLBACK DirectorySelectProc( HWND hWndDialog, UINT message,
																					WPARAM wParam, LPARAM lParam );

#endif // __WINDOWPROC_H_

