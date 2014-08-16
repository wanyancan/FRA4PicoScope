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
// Module: ScopeSelector.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ScopeSelector.h"
#include "StatusLog.h"
#include "ps2000aImpl.h"
#include "ps4000Impl.h"
#include "ps5000aImpl.h"
#include <iostream>
#include <sstream>

// Functions and types from the PicoScope SDKs that we'll define locally since the PS headers are incompatible with each other.
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps2000aEnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps3000aEnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps4000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps4000aEnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps5000aEnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps6000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth );

typedef enum enPS5000ADeviceResolution
{
    PS5000A_DR_8BIT,
    PS5000A_DR_12BIT,
    PS5000A_DR_14BIT,
    PS5000A_DR_15BIT,
    PS5000A_DR_16BIT
} PS5000A_DEVICE_RESOLUTION;

extern "C" __declspec(dllimport) short __stdcall ps2000_open_unit(void);
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps2000aOpenUnit( short * handle, char * serial );
extern "C" __declspec(dllimport) int16_t __stdcall ps3000_open_unit (void);
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps3000aOpenUnit( int16_t * handle, int8_t * serial );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps4000OpenUnitEx( int16_t * handle, int8_t * serial );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps5000OpenUnit( short * handle );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps5000aOpenUnit( int16_t *handle, int8_t *serial, PS5000A_DEVICE_RESOLUTION resolution );
extern "C" __declspec(dllimport) PICO_STATUS __stdcall ps6000OpenUnit( short * handle, char * serial );

// Put the enumeration functions in a array so they can be called from a loop
const ScopeSelector::psEnumerateUnits ScopeSelector::EnumerationFuncs[] =
{
    ScopeSelector::ps2000EnumerateUnits,
    ps2000aEnumerateUnits,
    ScopeSelector::ps3000EnumerateUnits,
    ps3000aEnumerateUnits,
    ps4000EnumerateUnits,
    ps4000aEnumerateUnits,
    ScopeSelector::ps5000EnumerateUnits,
    ps5000aEnumerateUnits,
    ps6000EnumerateUnits
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelector::ScopeSelector
//
// Purpose: Simple constructor
//
// Parameters: N/A
//
// Notes: Initializes selectedScope to NULL (none selected)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

ScopeSelector::ScopeSelector() : selectedScope(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelector::GetAvailableScopes
//
// Purpose: Gets a list of the scopes found through enumeration
//
// Parameters: _availableScopes: The list of available scopes
//             refresh: Whether to re-scan for scopes or just use the most recent list
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ScopeSelector::GetAvailableScopes( vector<AvalaibleScopeDescription_T>& _avaliableScopes, bool refresh )
{
    bool retVal = true;

    if (refresh == true)
    {
        PICO_STATUS status;
        int16_t count = 0;
        char serials[1024];
        short serialLength = 1024;
        AvalaibleScopeDescription_T desc;

        // reset the list of scopes
        availableScopes.resize(0);

        for (int idx = 0; idx < sizeof(EnumerationFuncs)/sizeof(psEnumerateUnits); idx++)
        {
            status = EnumerationFuncs[idx]( &count, (int8_t*)serials, &serialLength );
            if (count > 0)
            {
                // Parse out the scopes
                AddEnumeratedScopes( (ScopeDriverFamily_T)idx, string(serials) );
            }
        }

#ifdef TEST_SCOPE_SELECTION
        desc.compatible = true;
        desc.connected = false;
        desc.driverFamily = PS5000A;
        desc.wFamilyName = L"PS5000A";
        desc.model = PS5444A;
        desc.wSerialNumber = L"1234";
        desc.serialNumber = "1234";
        availableScopes.push_back(desc);

        desc.compatible = true;
        desc.connected = false;
        desc.driverFamily = PS2000;
        desc.wFamilyName = L"PS2000";
        desc.model = PS2203;
        desc.wSerialNumber = L"5678";
        desc.serialNumber = "5678";
        availableScopes.push_back(desc);
#endif
        // Enumeration functions don't find scopes already open
        if (selectedScope)
        {
            desc.connected = true;
            // No need to set desc.compatible since this scope can't be "re-opened"
            desc.driverFamily = selectedScope->family;
            // No need to set desc.serialNumber since this scope can't be "re-opened"
            desc.wFamilyName = wFamilyNames[selectedScope->family];
            selectedScope->GetSerialNumber(desc.wSerialNumber);
            availableScopes.push_back(desc);
        }
#ifdef TEST_SCOPE_SELECTION
        else
        {
            desc.compatible = true;
            desc.connected = true;
            desc.driverFamily = PS4000;
            desc.wFamilyName = L"PS4000";
            desc.model = PS4262;
            desc.wSerialNumber = L"CS89/234";
            desc.serialNumber = "CS89/234";
            availableScopes.push_back(desc);
        }
#endif

        if (retVal)
        {
            _avaliableScopes = availableScopes;
        }
    }
    else
    {
        retVal = true;
        _avaliableScopes = availableScopes;
    }
    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelector::OpenScope
//
// Purpose: Create a scope object based on the request
//
// Parameters: scope: Description of the scope to open
//             return: The created scope object.  May be NULL on failure.
//
// Notes: If it succeeds, it sets the currently selected scope.  
//        Some scope families are not implemented yet.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

PicoScope* ScopeSelector::OpenScope( AvalaibleScopeDescription_T scope )
{
    PICO_STATUS status;
    int16_t handle = -1;

    if (scope.driverFamily == PS2000)
    {
        // The scope is already open
        selectedScope = NULL; // For now, until we implement PS2000
    }
    else if (scope.driverFamily == PS2000A)
    {
        status = ps2000aOpenUnit( &handle, (char*)scope.serialNumber.c_str() );
        if (status != 0 || handle <= 0)
        {
            if (handle > 0)
            {
                selectedScope = new ps2000aImpl( handle );
                selectedScope->CloseUnit();
                delete selectedScope;
            }
            selectedScope = NULL;
        }
        else
        {
            selectedScope = new ps2000aImpl( handle );
        }
    }
    else if (scope.driverFamily == PS3000)
    {
        // The scope is already open
        selectedScope = NULL; // For now, until we implement PS3000
    }
    else if (scope.driverFamily == PS3000A)
    {
        status = ps3000aOpenUnit( &handle, (int8_t*)scope.serialNumber.c_str() );
        if (status != 0 || handle <= 0)
        {
            selectedScope = NULL;
        }
        else
        {
            //selectedScope = new ps3000aImpl( handle );
            selectedScope = NULL; // For now, until we implement PS3000a
        }
    }
    else if (scope.driverFamily == PS4000)
    {
        status = ps4000OpenUnitEx( &handle, (int8_t*)scope.serialNumber.c_str() );
        if (status != 0 || handle <= 0)
        {
            if (handle > 0)
            {
                selectedScope = new ps4000Impl( handle );
                selectedScope->CloseUnit();
                delete selectedScope;
            }
            selectedScope = NULL;
        }
        else
        {
            selectedScope = new ps4000Impl( handle );
        }
    }
    else if (scope.driverFamily == PS4000A)
    {
        selectedScope = NULL; // There are currently no compatible PS4000A family scopes
    }
    else if (scope.driverFamily == PS5000)
    {
        // The scope is already open
        selectedScope = NULL; // For now, until we implement PS5000
    }
    else if (scope.driverFamily == PS5000A)
    {
        status = ps5000aOpenUnit( &handle, (int8_t*)scope.serialNumber.c_str(), PS5000A_DR_15BIT );
        if (status != 0 || handle <= 0)
        {
            if (handle > 0)
            {
                selectedScope = new ps5000aImpl( handle );
                selectedScope->CloseUnit();
                delete selectedScope;
            }
            selectedScope = NULL;
        }
        else
        {
            selectedScope = new ps5000aImpl( handle );
        }
    }
    else if (scope.driverFamily == PS6000)
    {
        status = ps6000OpenUnit( &handle, NULL );
        if (status != 0 || handle <= 0)
        {
            selectedScope = NULL;
        }
        else
        {
            //selectedScope = new ps6000Impl( handle );
            selectedScope = NULL; // For now, until we implement PS6000
        }
    }
    else
    {
        selectedScope = NULL;
    }

    if (selectedScope)
    {
        selectedScope->family = scope.driverFamily;
        if (!InitializeScopeImpl())
        {
            delete selectedScope;
            selectedScope = NULL;
        }
        else
        {
            selectedScope->initialized = true;
        }
    }

    return selectedScope;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelector::GetSelectedScope
//
// Purpose: Simple accessor to get a pointer to the currently selected scope.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

PicoScope* ScopeSelector::GetSelectedScope(void)
{
    return selectedScope;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelector::AddEnumeratedScopes
//
// Purpose: Private function used to help add scopes to the available scopes list as they are enumerated.
//
// Parameters: family: The family of the scope (e.g. PS2000A)
//             serials: A list of comma separated scope serial numbers
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void ScopeSelector::AddEnumeratedScopes( ScopeDriverFamily_T family, string serials )
{
    istringstream ss(serials);
    wstringstream wss;

    AvalaibleScopeDescription_T desc;

    desc.driverFamily = family;
    desc.wFamilyName = wFamilyNames[family];

    while(getline(ss, desc.serialNumber, ','))
    {
        wss.clear();
        wss.str(L"");
        wss << desc.serialNumber.c_str();
        desc.wSerialNumber = wss.str();
        desc.compatible = true; // For now just set to true; later, set to the actual value, once supported by API.
        desc.connected = false;
        availableScopes.push_back(desc);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelector::InitializeScopeImpl
//
// Purpose: Private helper function used to initialize the newly created scope object.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ScopeSelector::InitializeScopeImpl( void )
{
    bool retVal = true;
    wstring modelName;
    int idx;

    // Get the scope model number so we can look it up in the implementation list.
    selectedScope->GetModel(modelName);

    //for( idx = 0; idx < sizeof(scopeImplTable)/sizeof(ScopeImplRecord_T); idx++ ) // MSVC bug
    for( idx = 0; idx < PS_MAX_SCOPE_MODELS; idx++ )
    {
        if (modelName == scopeImplTable[idx].modelName)
        {
            break;
        }
    }

    // if (idx == sizeof(scopeImplTable)/sizeof(ScopeImplRecord_T)) // MSVC bug
    if (idx == PS_MAX_SCOPE_MODELS)
    {
        retVal = false;
    }
    else
    {
        selectedScope->model = scopeImplTable[idx].model;
        selectedScope->numChannels = scopeImplTable[idx].numChannels;
        selectedScope->minFuncGenFreq = scopeImplTable[idx].minFuncGenFreq;
        selectedScope->maxFuncGenFreq = scopeImplTable[idx].maxFuncGenFreq;
        selectedScope->minFuncGenVpp = scopeImplTable[idx].minFuncGenVpp;
        selectedScope->maxFuncGenVpp = scopeImplTable[idx].maxFuncGenVpp;
        selectedScope->compatible = scopeImplTable[idx].compatible;

        if (!(selectedScope->InitializeScope()))
        {
            retVal = false;
        }
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelector::psXXXXEnumerateUnits
//
// Purpose: Enumeration functions for legacy series scopes who's libraries don't have such functions
//
// Parameters: See PicoScope Programmers Manual for API's of the same signature 
//
// Notes: Currently not implemented.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

PICO_STATUS ScopeSelector::ps2000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth )
{
    *count = 0;
    return PICO_OK;
}

PICO_STATUS ScopeSelector::ps3000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth )
{
    *count = 0;
    return PICO_OK;
}

PICO_STATUS ScopeSelector::ps5000EnumerateUnits( int16_t *count, int8_t  *serials, int16_t *serialLth )
{
    *count = 0;
    return PICO_OK;
}

const ScopeImplRecord_T ScopeSelector::scopeImplTable[] =
{
    { L"2204A", PS2204A, PS2000, 2, 0.0, 100000.0, 0.0, 4.0, true},
    { L"2205A", PS2205A, PS2000, 2, 0.0, 100000.0, 0.0, 4.0, true},
    { L"2206A", PS2206A, PS2000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"2207A", PS2207A, PS2000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"2208A", PS2208A, PS2000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"2205MSO", PS2205MSO, PS2000A, 2, 0.0, 100000.0, 0.0, 4.0, true},
    { L"3204A", PS3204A, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3204B", PS3204B, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3204MSO", PS3204MSO, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3205A", PS3205A, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3205B", PS3205B, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3205MSO", PS3205MSO, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3206A", PS3206A, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3206B", PS3206B, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3206MSO", PS3206MSO, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3207A", PS3207A, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3207B", PS3207B, PS3000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3404A", PS3404A, PS3000A, 4, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3404B", PS3404B, PS3000A, 4, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3405A", PS3405A, PS3000A, 4, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3405B", PS3405B, PS3000A, 4, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3406A", PS3406A, PS3000A, 4, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3406B", PS3406B, PS3000A, 4, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3425", PS3425, PS3000, 4, 0.0, 0.0, 0.0, 0.0, false},
    { L"4224", PS4224, PS4000, 2, 0.0, 0.0, 0.0, 0.0, false},
    { L"4224IEPE", PS4224IEPE, PS4000, 2, 0.0, 0.0, 0.0, 0.0, false},
    { L"4424", PS4424, PS4000, 4, 0.0, 0.0, 0.0, 0.0, false},
    { L"4262", PS4262, PS4000, 2, 0.0, 20000.0, 0.0, 2.0, true},
    { L"4824", PS4824, PS4000A, 8, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"5242A", PS5242A, PS5000A, 2, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5242B", PS5242B, PS5000A, 2, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5442A", PS5442A, PS5000A, 4, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5442B", PS5442B, PS5000A, 4, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5243A", PS5243A, PS5000A, 2, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5243B", PS5243B, PS5000A, 2, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5443A", PS5443A, PS5000A, 4, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5443B", PS5443B, PS5000A, 4, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5244A", PS5244A, PS5000A, 2, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5244B", PS5244B, PS5000A, 2, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5444A", PS5444A, PS5000A, 4, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"5444B", PS5444B, PS5000A, 4, 0.0, 20000000.0, 0.0, 4.0, true},
    { L"6402C", PS6402C, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6402D", PS6402D, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6403C", PS6403C, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6403D", PS6403D, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6404C", PS6404C, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6404D", PS6404D, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"2202", PS2202, PS2000, 2, 0.0, 0.0, 0.0, 0.0, false},
    { L"2203", PS2203, PS2000, 2, 0.0, 100000.0, 0.5, 4.0, true},
    { L"2204", PS2204, PS2000, 2, 0.0, 100000.0, 0.5, 4.0, true},
    { L"2205", PS2205, PS2000, 2, 0.0, 100000.0, 0.5, 4.0, true},
    { L"2206", PS2206, PS2000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"2207", PS2207, PS2000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"2208", PS2208, PS2000A, 2, 0.0, 1000000.0, 0.0, 4.0, true},
    { L"3204", PS3204, PS3000, 2, 0.0, 0.0, 0.0, 0.0, false},
    { L"3205", PS3205, PS3000, 2, 100.0, 1000000.0, 1.0, 1.0, true},
    { L"3206", PS3206, PS3000, 2, 100.0, 1000000.0, 1.0, 1.0, true},
    { L"3223", PS3223, PS3000, 2, 0.0, 0.0, 0.0, 0.0, false},
    { L"3224", PS3224, PS3000, 2, 0.0, 0.0, 0.0, 0.0, false},
    { L"3423", PS3423, PS3000, 4, 0.0, 0.0, 0.0, 0.0, false},
    { L"3424", PS3424, PS3000, 4, 0.0, 0.0, 0.0, 0.0, false},
    { L"4223", PS4223, PS4000, 2, 0.0, 0.0, 0.0, 0.0, false},
    { L"4423", PS4423, PS4000, 4, 0.0, 0.0, 0.0, 0.0, false},
    { L"4226", PS4226, PS4000, 2, 0.0, 100000.0, 0.5, 4.0, true},
    { L"4227", PS4227, PS4000, 2, 0.0, 100000.0, 0.5, 4.0, true},
    { L"5203", PS5203, PS5000, 2, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"5204", PS5204, PS5000, 2, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6402A", PS6402A, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6402B", PS6402B, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6403A", PS6403A, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6403B", PS6403B, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6404A", PS6404A, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true},
    { L"6404B", PS6404B, PS6000, 4, 0.0, 20000000.0, 0.5, 4.0, true}
};

const wstring ScopeSelector::wFamilyNames[] =
{
    L"PS2000",
    L"PS2000A",
    L"PS3000",
    L"PS3000A",
    L"PS4000",
    L"PS4000A",
    L"PS5000",
    L"PS5000A",
    L"PS6000"
};
