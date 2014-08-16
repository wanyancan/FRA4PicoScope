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
// Module: FRAPlotter.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Windows.h>

#include <stdint.h>
#include <vector>
#include <memory>
#include <boost/logic/tribool.hpp>
using namespace boost::logic;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: class FRAPlotter
//
// Purpose: This is the class supporting plotting of Frequency Response Analysis data
//
// Parameters: N/A
//
// Notes: This class uses PLplot and Windows Imaging Component to do the bulk of the work
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class FRAPlotter
{
    public:
        FRAPlotter(uint16_t width, uint16_t height);
        ~FRAPlotter(void);
        bool Initialize(void);
        void PlotFRA(double freqs[], double gains[], double phases[], int N);
        unique_ptr<uint8_t[]> GetScreenBitmapPlot32BppBGRA(void);
        unique_ptr<uint8_t[]> GetPNGBitmapPlot(size_t* size);

    private:
        uint16_t plotWidth;
        uint16_t plotHeight;
        vector<uint8_t> plotBmBuffer;

        const static uint8_t bytesPerPixel = 3; // 24bpp RGB for PLPlot "mem" driver

        typedef enum
        {
            CMD_MAKE_SCREEN_BITMAP_32BPP_BGRA,
            CMD_MAKE_FILE_BITMAP_PNG,
            CMD_EXIT
        } FraPlotterCommand_T;

        FraPlotterCommand_T cmd;
        uint8_t* workingBuffer;
        size_t workingBufferSize;
        tribool cmdResult;

        HANDLE hFraPlotterExecuteCommandEvent;
        HANDLE hFraPlotterCommandCompleteEvent;
        HANDLE hFraPlotterWorkerThread;

        static DWORD WINAPI WorkerThread(LPVOID lpThreadParameter);
};

