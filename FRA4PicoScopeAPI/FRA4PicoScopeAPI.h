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
// Module FRA4PicoScopeAPI.h: Header for the DLL build.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FRA4PICOSCOPE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FRA4PICOSCOPE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifdef FRA4PICOSCOPE_EXPORTS
#define FRA4PICOSCOPE_API __declspec(dllexport)
#else
#define FRA4PICOSCOPE_API __declspec(dllimport)
#endif

#include <stdint.h>
#include "PicoScopeFRA.h" // TBD remove from here when types get declared in FRA4PicoScopeInterfaceTypes.h
#include "FRA4PicoScopeInterfaceTypes.h"

// TBD - callback function registration 

FRA4PICOSCOPE_API bool SetScope( char* sn );

FRA4PICOSCOPE_API double GetMinFrequency(void);
FRA4PICOSCOPE_API bool ExecuteFRA(double startFreqHz, double stopFreqHz, int stepsPerDecade);
FRA4PICOSCOPE_API bool CancelFRA();
FRA4PICOSCOPE_API void SetFraSettings( SamplingMode_T samplingMode, bool sweepDescending, double phaseWrappingThreshold );
FRA4PICOSCOPE_API void SetFraTuning( double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                                      double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude, uint16_t minCyclesCaptured );
FRA4PICOSCOPE_API bool SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                                      int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                                      double signalVpp );
FRA4PICOSCOPE_API void GetResults( int* numSteps, double** freqsLogHz, double** gainsDb, double** phasesDeg, double** unwrappedPhasesDeg );
FRA4PICOSCOPE_API void EnableDiagnostics(wchar_t baseDataPath);
FRA4PICOSCOPE_API void DisableDiagnostics(void);