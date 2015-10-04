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
#include <tuple>
#include <stack>
#include <boost/logic/tribool.hpp>
#include "plplot.h"
using namespace boost::logic;

typedef struct
{
    tuple<bool,double,double> freqAxisScale;
    tuple<bool,double,double> gainAxisScale; 
    tuple<bool,double,double> phaseAxisScale;
    tuple<double,uint8_t,bool,bool> freqAxisIntervals;
    tuple<double,uint8_t,bool,bool> gainAxisIntervals; 
    tuple<double,uint8_t,bool,bool> phaseAxisIntervals;
    bool gainMasterIntervals; 
    bool phaseMasterIntervals;
} PlotScaleSettings_T;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: class FRAPlotter
//
// Purpose: This is the class supporting plotting of Frequency Response Analysis data.  It creates
//          image pixel buffers for clients to display or save images.
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
        void SetPlotSettings( bool plotGain, bool plotPhase, bool plotGainMargin, bool plotPhaseMargin, bool replot );
        void SetPlotData( double freqs[], double gains[], double phases[], int N, bool replot );
        void PlotFRA(double freqs[], double gains[], double phases[], int N, 
                     tuple<bool,double,double> freqAxisScale,
                     tuple<bool,double,double> gainAxisScale, 
                     tuple<bool,double,double> phaseAxisScale,
                     tuple<double,uint8_t,bool,bool> freqAxisIntervals,
                     tuple<double,uint8_t,bool,bool> gainAxisIntervals, 
                     tuple<double,uint8_t,bool,bool> phaseAxisIntervals,
                     bool gainMasterIntervals, bool phaseMasterIntervals);

        bool Zoom(tuple<int32_t,int32_t> plotOriginPoint,  // upper left screen coordinates
                  tuple<int32_t,int32_t> plotExtentPoint,  // lower right screen coordinates
                  tuple<int32_t,int32_t> zoomOriginPoint,  // screen coordinates
                  tuple<int32_t,int32_t> zoomExtentPoint); // screen coordinates

        bool UndoOneZoom(void);
        bool ZoomToOriginal(void);
        void GetPlotScaleSettings(PlotScaleSettings_T& plotSettings);
        bool PlotDataAvailable( void );

        unique_ptr<uint8_t[]> GetScreenBitmapPlot32BppBGRA(void);
        unique_ptr<uint8_t[]> GetPNGBitmapPlot(size_t* size);

    private:

        void PlotFRA(void);
        void CalculateGainAndPhaseMargins( void );

        uint16_t plotWidth;
        uint16_t plotHeight;
        bool plotDataAvailable;

        vector<uint8_t> plotBmBuffer;

        const static uint8_t bytesPerPixel = 3; // 24bpp RGB for PLPlot "mem" driver

        // Data describing the current plot
        bool mPlotGain, mPlotPhase, mPlotGainMargin, mPlotPhaseMargin;
        double currentFreqAxisMin; // Log Hz
        double currentFreqAxisMax; // Log Hz
        double currentGainAxisMin;
        double currentGainAxisMax;
        double currentPhaseAxisMin;
        double currentPhaseAxisMax;
        double currentFreqAxisMajorTickInterval;
        int32_t currentFreqAxisMinorTicksPerMajorInterval;
        double currentGainAxisMajorTickInterval;
        int32_t currentGainAxisMinorTicksPerMajorInterval;
        bool currentGainMasterIntervals;
        double currentPhaseAxisMajorTickInterval;
        int32_t currentPhaseAxisMinorTicksPerMajorInterval;
        bool currentPhaseMasterIntervals;
        string currentFreqAxisOpts, currentGainAxisOpts, currentPhaseAxisOpts;
        vector<double> currentFreqs, currentGains, currentPhases;
        vector<pair<double,double>> gainMargins;
        vector<pair<double,double>> phaseMargins;

        // A stack of zoom values (top is current)
        stack<tuple<tuple<bool,double,double>,tuple<bool,double,double>,tuple<bool,double,double>>> zoomStack;

        PlotScaleSettings_T currentPlotScaleSettings;

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
        static int HandlePLplotError(const char* error);

        static void LabelYAxis(PLINT axis, PLFLT value, char* labelText, PLINT length, PLPointer labelData);
        void LabelYAxisExponent( PLINT scale, char* side );
        static PLINT widestYAxisLabel;
        static bool scientific;
        static PLINT yAxisScale;

        const static double viewPortLeftEdge;
        const static double viewPortRightEdge;
        const static double viewPortBottomEdge;
        const static double viewPortTopEdge;
};

