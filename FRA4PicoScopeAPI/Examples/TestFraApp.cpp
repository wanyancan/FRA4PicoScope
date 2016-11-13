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
// Module TestFraApp.cpp: Test Application for the FRA4PicoScope API DLL.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <vector>
#include "FRA4PicoScopeAPI.h"

FRA_STATUS_T status;

int main()
{
    if (!Initialize())
    {
        std::cout << "Error initializing" << std::endl;
        return -1;
    }

    EnableMessageLog(true);

    if (!SetScope(""))
    {
        std::cout << "Error setting scope" << std::endl;
        return -1;
    }

    if (!SetupChannels( PS_CHANNEL_A, PS_AC, ATTEN_1X, 0.0,
                        PS_CHANNEL_B, PS_AC, ATTEN_1X, 0.0, 4.0 ))
    {
        std::cout << "Error setting channels" << std::endl;
        return -1;
    }

    if (!StartFRA(100.0, 10000.0, 10))
    {
        std::cout << "Error starting FRA" << std::endl;
        return -1;
    }

    while (FRA_STATUS_IN_PROGRESS == (status=GetFraStatus())) {}

    if (status == FRA_STATUS_COMPLETE)
    {
        int numSteps;
        std::vector<double> freqsLogHz;
        std::vector<double> gainsDb;
        std::vector<double> phasesDeg;
        std::vector<double> unwrappedPhasesDeg;
        
        numSteps = GetNumSteps();

        freqsLogHz.resize(numSteps);
        gainsDb.resize(numSteps);
        phasesDeg.resize(numSteps);
        unwrappedPhasesDeg.resize(numSteps);

        GetResults( freqsLogHz.data(), gainsDb.data(), phasesDeg.data(), unwrappedPhasesDeg.data() );

        std::cout << "Frequency log(Hz), Gain dB, Phase degrees, Unwrapped phase degrees" << std::endl;
        for (int i = 0; i < numSteps; i++)
        {
            std::cout << freqsLogHz[i] << ", " << gainsDb[i] << ", " << phasesDeg[i] << ", " << unwrappedPhasesDeg[i] << std::endl;
        }
    }
    else
    {
        std::cout << "Error completing FRA; status = " << status << std::endl;
        return -1;
    }

    std::wcout << std::endl << std::endl << GetMessageLog();

    Cleanup();

    return 0;
}

