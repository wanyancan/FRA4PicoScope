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
// Module: PicoScopeInterface.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "StdAfx.h"
#include "picoStatus.h"
#include <string>
#include <vector>

typedef enum
{
    PS2000,
    PS2000A,
    PS3000,
    PS3000A,
    PS4000,
    PS4000A,
    PS5000,
    PS5000A,
    PS6000,
    PS_NO_FAMILY
} ScopeDriverFamily_T;

typedef enum
{
    PS2204A,
    PS2205A,
    PS2206A,
    PS2207A,
    PS2208A,
    PS2206B,
    PS2207B,
    PS2208B,
    PS2405A,
    PS2406B,
    PS2407B,
    PS2408B,
    PS2205MSO,
    PS2205AMSO,
    PS2206BMSO,
    PS2207BMSO,
    PS2208BMSO,
    PS3203D,
    PS3203DMSO,
    PS3204A,
    PS3204B,
    PS3204MSO,
    PS3204D,
    PS3204DMSO,
    PS3205A,
    PS3205B,
    PS3205MSO,
    PS3205D,
    PS3205DMSO,
    PS3206A,
    PS3206B,
    PS3206MSO,
    PS3206D,
    PS3206DMSO,
    PS3207A,
    PS3207B,
    PS3403D,
    PS3403DMSO,
    PS3404A,
    PS3404B,
    PS3404D,
    PS3404DMSO,
    PS3405A,
    PS3405B,
    PS3405D,
    PS3405DMSO,
    PS3406A,
    PS3406B,
    PS3406D,
    PS3406DMSO,
    PS3425,
    PS4224,
    PS4224IEPE,
    PS4424,
    PS4262,
    PS4824,
    PS5242A,
    PS5242B,
    PS5442A,
    PS5442B,
    PS5243A,
    PS5243B,
    PS5443A,
    PS5443B,
    PS5244A,
    PS5244B,
    PS5444A,
    PS5444B,
    PS6402C,
    PS6402D,
    PS6403C,
    PS6403D,
    PS6404C,
    PS6404D,
    PS6407,
    PS2202,
    PS2203,
    PS2204,
    PS2205,
    PS2206,
    PS2207,
    PS2208,
    PS3204,
    PS3205,
    PS3206,
    PS3223,
    PS3224,
    PS3423,
    PS3424,
    PS4223,
    PS4423,
    PS4226,
    PS4227,
    PS5203,
    PS5204,
    PS6402,
    PS6402A,
    PS6402B,
    PS6403,
    PS6403A,
    PS6403B,
    PS6404,
    PS6404A,
    PS6404B,
    PS_MAX_SCOPE_MODELS = PS6404B,
    PS_NO_MODEL
} ScopeModel_T;

typedef uint8_t PS_RANGE;

typedef struct
{
    double rangeVolts; // expressed as +/- X volts
    double ratioUp; // multiplier for how the signal will scale when increasing the size of the range, 0.0 if NA
    double ratioDown; // multiplier for how the signal will scale when decreasing the size of the range, 0.0 if NA
    wchar_t* name;
} RANGE_INFO_T;

typedef enum
{
    PS_CHANNEL_A,
    PS_CHANNEL_B,
    PS_CHANNEL_C,
    PS_CHANNEL_D,
    PS_CHANNEL_E,
    PS_CHANNEL_F,
    PS_CHANNEL_G,
    PS_CHANNEL_H,
    PS_CHANNEL_INVALID
} PS_CHANNEL;

typedef enum
{
    PS_AC,
    PS_DC,
    PS_DC_1M = PS_DC,
    PS_DC_50R
} PS_COUPLING;

typedef void (__stdcall *psBlockReady)
(
    int16_t      handle,
    PICO_STATUS  status,
    void         *pParameter
);

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: class PicoScope
//
// Purpose: This is a pure virtual interface class, defining a common interface to PicoScope objects
//          for the purposes of supporting FRA.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class PicoScope
{
        friend class ScopeSelector;

    public:
        PicoScope() : initialized(false), model(PS_NO_MODEL), family(PS_NO_FAMILY), numAvailableChannels(2), minFuncGenFreq(0.0), maxFuncGenFreq(0.0), minFuncGenVpp(0.0), maxFuncGenVpp(0.0), signalGeneratorPrecision(0.0), compatible(false) {};
        virtual ~PicoScope() {};

        class PicoPowerChange : public exception
        {
            public:
                PicoPowerChange(PICO_STATUS _state) { state = _state; }
                PICO_STATUS GetState(void) const { return state; }
            private:
                PICO_STATUS state;
        };

        virtual bool Initialized(void) = 0;
        virtual uint8_t GetNumChannels( void ) = 0;
        virtual void GetAvailableCouplings( vector<wstring>& couplingText ) = 0;
        virtual uint32_t GetNoiseRejectModeTimebase( void ) = 0;
        virtual double GetNoiseRejectModeSampleRate( void ) = 0;
        virtual double GetSignalGeneratorPrecision( void ) = 0;
        virtual double GetClosestSignalGeneratorFrequency( double requestedFreq ) = 0;
        virtual uint32_t GetMaxDataRequestSize( void ) = 0;
        virtual PS_RANGE GetMinRange( PS_COUPLING coupling ) = 0;
        virtual PS_RANGE GetMaxRange( PS_COUPLING coupling ) = 0;
        virtual int16_t GetMaxValue( void ) = 0;
        virtual double GetMinFuncGenFreq( void ) = 0;
        virtual double GetMaxFuncGenFreq( void ) = 0;
        virtual double GetMinFuncGenVpp( void ) = 0;
        virtual double GetMaxFuncGenVpp( void ) = 0;
        virtual double GetMinNonZeroFuncGenVpp( void ) = 0;
        virtual bool IsCompatible( void ) = 0;
        virtual bool GetModel( wstring &model ) = 0;
        virtual bool GetSerialNumber( wstring &sn ) = 0;
        virtual bool SetupChannel( PS_CHANNEL channel, PS_COUPLING coupling, PS_RANGE range, float offset ) = 0;
        virtual bool DisableAllDigitalChannels( void ) = 0;
        virtual bool DisableChannel( PS_CHANNEL channel ) = 0;
        virtual bool SetSignalGenerator( double vPP, double frequency ) = 0;
        virtual bool DisableSignalGenerator( void ) = 0;
        virtual bool DisableChannelTriggers( void ) = 0;
        virtual bool GetMaxSamples( uint32_t* maxSamples ) = 0;
        virtual bool GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase ) = 0;
        virtual bool RunBlock( int32_t numSamples, uint32_t timebase, int32_t *timeIndisposedMs, psBlockReady lpReady, void *pParameter ) = 0;
        virtual void SetChannelDesignations( PS_CHANNEL inputChannel, PS_CHANNEL outputChannel ) = 0;
        virtual bool GetData( uint32_t numSamples, uint32_t startIndex, vector<int16_t>** inputBuffer, vector<int16_t>** outputBuffer ) = 0;
        virtual bool GetCompressedData( uint32_t numSamples, 
                                        vector<int16_t>& inputCompressedMinBuffer, vector<int16_t>& outputCompressedMinBuffer,
                                        vector<int16_t>& inputCompressedMaxBuffer, vector<int16_t>& outputCompressedMaxBuffer ) = 0;
        virtual bool GetPeakValues( uint16_t& inputPeak, uint16_t& outputPeak, bool& inputOv, bool& outputOv ) = 0;
        virtual bool ChangePower( PICO_STATUS powerState ) = 0;
        virtual bool Close( void ) = 0;

        virtual const RANGE_INFO_T* GetRangeCaps( void ) = 0;

    private:
        virtual bool InitializeScope( void ) = 0;
        virtual bool IsUSB3_0Connection(void) = 0;

    protected:
        bool initialized;
        ScopeModel_T model;
        ScopeDriverFamily_T family;
        uint8_t numAvailableChannels;
        double minFuncGenFreq;
        double maxFuncGenFreq;
        double minFuncGenVpp;
        double maxFuncGenVpp;
        double signalGeneratorPrecision;
        bool compatible;
};