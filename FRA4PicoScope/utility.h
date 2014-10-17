//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2014 by Aaron Hexamer
//
// This file is part of the Frequency Response Analyzer for PicoScope program.
//
// Frequency Response Analyzer for PicoScope is free software: you can 
// redistribute it and/or modify it under the terms of the GNU General Public 
// License as published by the Free Software Foundation, either version 3 of 
// the License, or (at your option) any later version.
//
// Frequency Response Analyzer for PicoScope is distributed in the hope that 
// it will be useful,but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Frequency Response Analyzer for PicoScope.  If not, see <http://www.gnu.org/licenses/>.
//
// Module: utility.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <string>
#include <sstream>
#include <stdint.h>
#include <boost/numeric/conversion/cast.hpp> 
#include <limits>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: WStringTo[type]
//
// Purpose: Strictly convert a string to a number, while indicating whether the string was properly
//          formatted.
//
// Parameters: [in] myString: The string to convert
//             [out] d/i/u: The resulting number
//             [out] return: Whether the conversion succeeded.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

inline bool WStringToDouble( wstring myString, double& d )
{
    wstringstream wss(myString);
    wss >> noskipws >> d; // Consider leading whitespace invalid
    // Valid only if whole string was consumed and neither failbit nor badbit is set
    return (wss.eof() && !wss.fail());
}

inline bool WStringToInt16( wstring myString, int16_t& i )
{
    wstringstream wss(myString);
    wss >> noskipws >> i; // Consider leading whitespace invalid
    // Valid only if whole string was consumed and neither failbit nor badbit is set
    return (wss.eof() && !wss.fail());
}

inline bool WStringToUint8( wstring myString, uint8_t& u )
{
    // Need to use a uint16_t because extract operator for a uint8_t will
    // treat it as a character extract
    uint16_t u16;
    wstringstream wss(myString);
    wss >> noskipws >> u16; // Consider leading whitespace invalid
    // Valid only if whole string was consumed and neither failbit nor badbit is set
    if (wss.eof() && !wss.fail())
    {
        // Valid only if it can fit in a uint8_t
        if (u16 <= UINT8_MAX)
        {
            u = (uint8_t)u16;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: saturation_cast<Target,Source>
//
// Purpose: Perform a numerical cast with Boost numeric_cast, but bound the results to fit into the
//          destination type
//
// Parameters: [in] src: The source number to cast
//             [out] return: The result of the cast
//
// Notes: From http://stackoverflow.com/questions/4424168/using-boost-numeric-cast with minor mods
//
///////////////////////////////////////////////////////////////////////////////////////////////////

using boost::numeric_cast;
using boost::numeric::bad_numeric_cast;
using boost::numeric::positive_overflow;
using boost::numeric::negative_overflow;

template<typename Target, typename Source>
Target saturation_cast(Source src) 
{
    try 
    {
        return numeric_cast<Target>(src);
    }
    catch (const negative_overflow &e) 
    {
        UNREFERENCED_PARAMETER(e);
        // Use of parens to avoid conflict with min macro
        return (numeric_limits<Target>::min)();
    }
    catch (const positive_overflow &e) 
    {
        UNREFERENCED_PARAMETER(e);
        // Use of parens to avoid conflict with max macro
        return (numeric_limits<Target>::max)();
    }
}