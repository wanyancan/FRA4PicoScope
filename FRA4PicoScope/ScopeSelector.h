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
// Module: ScopeSelector.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "picoStatus.h"
#include "PicoScopeInterface.h"
#include <string>
#include <vector>
#include <unordered_map>

// Struct to describe a scope that's available to open
typedef struct AvailableScopeDescription
{
    ScopeModel_T model;
    ScopeDriverFamily_T driverFamily;
    wstring wFamilyName;
    wstring wSerialNumber;
    string serialNumber;
    bool compatible;
    bool connected;
    AvailableScopeDescription() : model(PS_NO_MODEL), driverFamily(PS_NO_FAMILY), 
                                  wFamilyName(L""), wSerialNumber(L""), serialNumber(""), 
                                  compatible(false), connected(false)
    {}
} AvailableScopeDescription_T;

// Struct to describe a scope's implementation capabilites
typedef struct
{
    wstring modelName;
    ScopeModel_T model;
    ScopeDriverFamily_T driverFamily;
    uint8_t numChannels;
    double minFuncGenFreq;
    double maxFuncGenFreq;
    double minFuncGenVpp;
    double maxFuncGenVpp;
    bool compatible;
} ScopeImplRecord_T;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: class ScopeSelector
//
// Purpose: This is a factory class, providing services to find and open PicoScope objects
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class ScopeSelector
{
    public:
        ScopeSelector();
        ~ScopeSelector();

        // Gets information about available scopes
        bool GetAvailableScopes( vector<AvailableScopeDescription_T>& _avaliableScopes, bool refresh = true );

        // Factory method to create scopes for the PicoScopeFRA to use
        PicoScope* OpenScope( AvailableScopeDescription_T scope );

        PicoScope* GetSelectedScope( void );

    private:
        PicoScope* selectedScope;
        vector<AvailableScopeDescription_T> availableScopes;
        static const ScopeImplRecord_T scopeImplTable[];
        static const wstring wFamilyNames[];

        void AddEnumeratedScopes( ScopeDriverFamily_T family, string serials );
        void ScopeSelector::CloseScopesFromSimulatedEnumeration( AvailableScopeDescription_T* doNotClose );
        bool InitializeScopeImpl( void );

        typedef PICO_STATUS  (__stdcall *psEnumerateUnits)( int16_t *count, int8_t  *serials, int16_t *serialLth );

        static const psEnumerateUnits EnumerationFuncs[];
        // Enumeration functions we'll define which will emulate the ones provided by the "A" series API's
        static PICO_STATUS __stdcall ps2000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );
        static PICO_STATUS __stdcall ps3000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );
        static PICO_STATUS __stdcall ps5000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );

        PICO_STATUS PS2000OpenUnit( int16_t* handle, int8_t* serial );
        PICO_STATUS PS3000OpenUnit( int16_t* handle, int8_t* serial );
        PICO_STATUS PS5000OpenUnit( int16_t* handle, int8_t* serial );

        static unordered_map<string, int16_t> ps2000Scopes;
        static unordered_map<string, int16_t> ps3000Scopes;
        static unordered_map<string, int16_t> ps5000Scopes;

        static void LogPicoApiCall( wstringstream& scopeStatusText );
        template <typename First, typename... Rest> static void LogPicoApiCall( wstringstream& scopeStatusText, First first, Rest... rest );
        void LogEnumerationCall( uint8_t idx, int16_t* count, int8_t* serials, int16_t* serialLth );
};