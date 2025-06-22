/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
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

// This file contains macros to help upgrade the code for newer cpp standards.
#pragma once

#if __cplusplus >= 201703L
#define NOEXCEPT_17 noexcept
#define REGISTER
#define FALLTHROUGH [[fallthrough]]
#else
#define NOEXCEPT_17
#define REGISTER register
#define FALLTHROUGH
#endif

// noexcept for methods of IUNKNOWN interface
#if defined(_MSC_VER)
#define IUNKNOWN_NOEXCEPT NOEXCEPT_17
#else
#define IUNKNOWN_NOEXCEPT
#endif

#if __cplusplus >= 201103L
  #define CPP_11(code) code
  #define CONSTEXPR constexpr
#else
  #define CPP_11(code)
  #define CONSTEXPR
#endif

#if __cplusplus < 201103L
#define static_assert(expr, msg)
#endif
