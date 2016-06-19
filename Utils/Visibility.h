/*
 *  Copyright 2008 Fabrice Colin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _VISIBILITY_H
#define _VISIBILITY_H

#ifndef PINOT_EXPORT
// The following comes from http://gcc.gnu.org/wiki/Visibility
// with some modifications
#ifdef _MSC_VER
  #ifdef BUILDING_DLL
    #define PINOT_EXPORT __declspec(dllexport)
  #else
    #define PINOT_EXPORT __declspec(dllimport)
  #endif
#else
  #if defined __GNUC__ && (__GNUC__ >= 4)
    #define PINOT_EXPORT __attribute__ ((visibility("default")))
  #else
    #define PINOT_EXPORT
  #endif
#endif
#endif

#endif // _VISIBILITY_H
