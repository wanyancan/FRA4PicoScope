//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2014, 2015 by Aaron Hexamer
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
// Module: ps4000aImpl.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PicoScopeInterface.h"

class ps4000aImpl : public PicoScope
{
    public:
        ps4000aImpl( int16_t _handle, PICO_STATUS _initialPowerState );
        ~ps4000aImpl();
    private:
        uint8_t numActualChannels;
        PICO_STATUS initialPowerState;

#define NEW_PS_DRIVER_MODEL
#include "psCommonImpl.h"

};