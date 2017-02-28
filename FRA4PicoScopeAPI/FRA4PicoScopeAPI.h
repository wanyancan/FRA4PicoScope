//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2016 by Aaron Hexamer
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
// Module FRA4PicoScopeAPI.h: Header for the FRA4PicoScope API DLL.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FRA4PICOSCOPE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FRA4PICOSCOPE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#ifdef FRA4PICOSCOPEAPI_EXPORTS
#define FRA4PICOSCOPE_API EXTERN_C __declspec(dllexport)
#else
#define FRA4PICOSCOPE_API EXTERN_C __declspec(dllimport)
#endif

#include <stdint.h>
#include "FRA4PicoScopeInterfaceTypes.h"

FRA4PICOSCOPE_API bool __stdcall SetScope( char* sn );
FRA4PICOSCOPE_API double __stdcall GetMinFrequency( void );
FRA4PICOSCOPE_API bool __stdcall StartFRA( double startFreqHz, double stopFreqHz, int stepsPerDecade );
FRA4PICOSCOPE_API bool __stdcall CancelFRA( void );
FRA4PICOSCOPE_API FRA_STATUS_T __stdcall GetFraStatus( void );
FRA4PICOSCOPE_API void __stdcall SetFraSettings( SamplingMode_T samplingMode, bool adaptiveStimulusMode, double targetResponseAmplitude,
                                                 bool sweepDescending, double phaseWrappingThreshold );
FRA4PICOSCOPE_API void __stdcall SetFraTuning( double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                                               double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude,
                                               uint8_t adaptiveStimulusTriesPerStep, double targetResponseAmplitudeTolerance, uint16_t minCyclesCaptured,
                                               uint16_t maxCyclesCaptured, uint16_t lowNoiseOversampling );
FRA4PICOSCOPE_API bool __stdcall SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                                                int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                                                double initialStimulusVpp, double maxStimulusVpp );
FRA4PICOSCOPE_API int __stdcall GetNumSteps( void );
FRA4PICOSCOPE_API void __stdcall GetResults( double* freqsLogHz, double* gainsDb, double* phasesDeg, double* unwrappedPhasesDeg );
FRA4PICOSCOPE_API void __stdcall EnableDiagnostics( wchar_t* baseDataPath );
FRA4PICOSCOPE_API void __stdcall DisableDiagnostics( void );
FRA4PICOSCOPE_API void __stdcall AutoClearMessageLog( bool bAutoClear );
FRA4PICOSCOPE_API void __stdcall EnableMessageLog( bool bEnable );
FRA4PICOSCOPE_API const wchar_t* __stdcall GetMessageLog( void );
FRA4PICOSCOPE_API void __stdcall ClearMessageLog( void );
FRA4PICOSCOPE_API void __stdcall SetCallback( FRA_STATUS_CALLBACK fraCb );
FRA4PICOSCOPE_API bool __stdcall Initialize( void );
FRA4PICOSCOPE_API void __stdcall Cleanup( void );