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
// Module: psCommonImpl.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

public:
bool Initialized( void );
uint8_t GetNumChannels( void );
uint32_t GetNoiseRejectModeTimebase( void );
double GetNoiseRejectModeSampleRate( void );
double GetSignalGeneratorPrecision( void );
PS_RANGE GetMinRange( void );
PS_RANGE GetMaxRange( void );
int16_t GetMaxValue( void );
double GetMinFuncGenFreq( void );
double GetMaxFuncGenFreq( void );
double GetMinFuncGenVpp( void );
double GetMaxFuncGenVpp( void );
bool IsCompatible( void );
bool GetModel( wstring &model );
bool GetSerialNumber( wstring &sn );
bool SetupChannel( PS_CHANNEL channel, PS_COUPLING coupling, PS_RANGE range, float offset );
bool DisableChannel( PS_CHANNEL channel );
bool SetSignalGenerator( double vPP, double frequency );
bool DisableSignalGenerator( void );
bool DisableChannelTriggers( void );
bool GetMaxSamples( uint32_t* maxSamples );
bool GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase );
bool RunBlock( int32_t numSamples, uint32_t timebase, int32_t *timeIndisposedMs, psBlockReady lpReady, void *pParameter );
// Abstracts set buffers and get data
bool GetData( uint32_t numSamples, uint32_t downsampleTo, PS_CHANNEL inputChannel, PS_CHANNEL outputChannel,
              vector<int16_t>& inputFullBuffer, vector<int16_t>& outputFullBuffer,
              vector<int16_t>& inputCompressedMinBuffer, vector<int16_t>& outputCompressedMinBuffer,
              vector<int16_t>& inputCompressedMaxBuffer, vector<int16_t>& outputCompressedMaxBuffer, int16_t& inputAbsMax, int16_t& outputAbsMax,
              bool* inputOv, bool* outputOv );
bool Close( void );
const RANGE_INFO_T* GetRangeCaps( void );
private:
static const RANGE_INFO_T rangeInfo[];
int16_t handle;
PS_RANGE minRange;
PS_RANGE maxRange;
uint32_t timebaseNoiseRejectMode;
double fSampNoiseRejectMode;
#if !defined(NEW_PS_DRIVER_MODEL)
static DWORD WINAPI CheckStatus(LPVOID lpThreadParameter);
bool InitStatusChecking(void);
HANDLE hCheckStatusEvent;
HANDLE hCheckStatusThread;
psBlockReady readyCB;
int32_t currentTimeIndisposedMs;
void* cbParam;
#endif
bool InitializeScope( void );