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
bool Connected(void);
uint8_t GetNumChannels( void );
void GetAvailableCouplings( vector<wstring>& couplingText );
uint32_t GetMinTimebase( void );
uint32_t GetMaxTimebase( void );
uint32_t GetNoiseRejectModeTimebase( void );
void SetDesiredNoiseRejectModeTimebase( uint32_t timebase );
uint32_t GetDefaultNoiseRejectModeTimebase( void );
double GetNoiseRejectModeSampleRate( void );
double GetSignalGeneratorPrecision( void );
double GetClosestSignalGeneratorFrequency( double requestedFreq );
uint32_t GetMaxDataRequestSize( void );
PS_RANGE GetMinRange( PS_COUPLING coupling );
PS_RANGE GetMaxRange( PS_COUPLING coupling );
int16_t GetMaxValue( void );
double GetMinFuncGenFreq( void );
double GetMaxFuncGenFreq( void );
double GetMinFuncGenVpp( void );
double GetMaxFuncGenVpp( void );
double GetMinNonZeroFuncGenVpp( void );
bool IsCompatible( void );
bool GetModel( wstring &model );
bool GetSerialNumber( wstring &sn );
bool SetupChannel( PS_CHANNEL channel, PS_COUPLING coupling, PS_RANGE range, float offset );
bool DisableAllDigitalChannels( void );
bool DisableChannel( PS_CHANNEL channel );
bool SetSignalGenerator( double vPP, double offset, double frequency );
bool DisableSignalGenerator( void );
bool DisableChannelTriggers( void );
bool GetMaxSamples( uint32_t* maxSamples );
bool GetTimebase( double desiredFrequency, double* actualFrequency, uint32_t* timebase );
bool GetFrequencyFromTimebase( uint32_t timebase, double &frequency );
bool RunBlock( int32_t numSamples, uint32_t timebase, int32_t *timeIndisposedMs, psBlockReady lpReady, void *pParameter );
void SetChannelDesignations( PS_CHANNEL inputChannel, PS_CHANNEL outputChannel );
bool GetData( uint32_t numSamples, uint32_t startIndex, vector<int16_t>** inputBuffer, vector<int16_t>** outputBuffer );
bool GetCompressedData( uint32_t numSamples, 
                        vector<int16_t>& inputCompressedMinBuffer, vector<int16_t>& outputCompressedMinBuffer,
                        vector<int16_t>& inputCompressedMaxBuffer, vector<int16_t>& outputCompressedMaxBuffer );
bool GetPeakValues( uint16_t& inputPeak, uint16_t& outputPeak, bool& inputOv, bool& outputOv );
bool ChangePower(PICO_STATUS powerState);
bool CancelCapture( void );
bool Close( void );
const RANGE_INFO_T* GetRangeCaps( void );
private:
static const RANGE_INFO_T rangeInfo[];
int16_t handle;
PS_RANGE minRange;
PS_RANGE maxRange;
uint32_t minTimebase;
uint32_t maxTimebase;
uint32_t defaultTimebaseNoiseRejectMode;
uint32_t timebaseNoiseRejectMode;
PS_CHANNEL mInputChannel;
PS_CHANNEL mOutputChannel;
vector<int16_t> mInputBuffer;
vector<int16_t> mOutputBuffer;
bool buffersDirty;
uint32_t mNumSamples;
static const uint32_t maxDataRequestSize;
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
bool IsUSB3_0Connection();
void LogPicoApiCall( wstringstream& fraStatusText );
template <typename First, typename... Rest> void LogPicoApiCall( wstringstream& fraStatusText, First first, Rest... rest );