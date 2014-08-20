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
// Module: FRAPlotter.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FRAPlotter.h"
#include "plplot.h"
#include <algorithm>
#include <Wincodec.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SafeRelease
//
// Purpose: Release Com resource if it is allocated
//
// Parameters: [in] ppT - Pointer to the object to release
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FRAPlotter::FRAPlotter
//
// Purpose: Constructor
//
// Parameters: [in] width - Width of the plot
//             [in] height - Height of the plot
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

FRAPlotter::FRAPlotter( uint16_t width, uint16_t height )
{
    plotWidth = width;
    plotHeight = height;
    plotBmBuffer.resize(plotWidth*plotHeight*bytesPerPixel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FRAPlotter::Initialize
//
// Purpose: Initialize the FRAPlotter object, creating all the appropriate thread structures to 
//          support the services it provides.
//
// Parameters:
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool FRAPlotter::Initialize(void)
{
    hFraPlotterExecuteCommandEvent = CreateEventW( NULL, false, false, L"ExecutePlotterCommand" );

    if ((HANDLE)NULL == hFraPlotterExecuteCommandEvent)
    {
        return false;
    }

    if( !ResetEvent( hFraPlotterExecuteCommandEvent ) )
    {
        return false;
    }

    hFraPlotterCommandCompleteEvent = CreateEventW( NULL, false, false, L"PlotterCommandComplete" );

    if ((HANDLE)NULL == hFraPlotterCommandCompleteEvent)
    {
        return false;
    }

    if( !ResetEvent( hFraPlotterCommandCompleteEvent ) )
    {
        return false;
    }

    hFraPlotterWorkerThread = CreateThread( NULL, 0, WorkerThread, this, 0, NULL );

    if (0 != GetLastError())
    {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FRAPlotter::~FRAPlotter
//
// Purpose: Destructor
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

FRAPlotter::~FRAPlotter(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FRAPlotter::PlotFRA
//
// Purpose: Plots the FRA data to an internal buffer
//
// Parameters: [in] freqs - Frequencies at which the FRA was executed
//             [in] gains - Gains from the FRA execution
//             [in] phases - Phases from the FRA execution
//             [in] N - Number of data points in the arrays supplied
//             [in] freqAxisScale - a tuple where the first element is whether to autoscale,
//                                  and if false, the second and third are the min and max values
//             [in] gainAxisScale - a tuple where the first element is whether to autoscale,
//                                  and if false, the second and third are the min and max values
//             [in] phaseAxisScale - a tuple where the first element is whether to autoscale,
//                                   and if false, the second and third are the min and max values
//             [in] freqAxisIntervals - a tuple where the first two elements are the major and 
//                                      minor intervals, and the next two elements indicate
//                                      whether to draw grids
//             [in] gainAxisIntervals - a tuple where the first two elements are the major and 
//                                      minor intervals, and the next two elements indicate
//                                      whether to draw grids
//             [in] phaseAxisIntervals - a tuple where the first two elements are the major and 
//                                       minor intervals, and the next two elements indicate
//                                       whether to draw grids
//
// Notes: Use of 0.0 for intervals results in PLplot automatically calculating an interval.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void FRAPlotter::PlotFRA(double freqs[], double gains[], double phases[], int N, 
                         tuple<bool,double,double> freqAxisScale,
                         tuple<bool,double,double> gainAxisScale, 
                         tuple<bool,double,double> phaseAxisScale,
                         tuple<double,uint8_t,bool,bool> freqAxisIntervals,
                         tuple<double,uint8_t,bool,bool> gainAxisIntervals, 
                         tuple<double,uint8_t,bool,bool> phaseAxisIntervals)
{

    double minGain, maxGain;
    double minPhase, maxPhase;
    double minFreq, maxFreq;
    string freqAxisOpts = "bclnst";
    string gainAxisOpts = "bnstv";
    string phaseAxisOpts = "cmstv";

    for (uint8_t &bufferElem : plotBmBuffer) // range based for from C++11 not available in VS2010
    {
        bufferElem = 230; // Set to a light gray background
    }

    plsdev( "mem" );
    plsmem(plotWidth, plotHeight, plotBmBuffer.data());
    plinit();

    // Compute axis mins and maxes

    if (get<0>(freqAxisScale)) // autoscale is true
    {
        minFreq = *min_element(freqs,freqs+N);
        maxFreq = *max_element(freqs,freqs+N);
        minFreq -= 0.1;// * (maxFreq - minFreq);
        maxFreq += 0.1;// * (maxFreq - minFreq);
    }
    else
    {
        minFreq = get<1>(freqAxisScale);
        maxFreq = get<2>(freqAxisScale);
    }

    if (get<0>(gainAxisScale)) // autoscale is true
    {
        minGain = *min_element(gains,gains+N);
        maxGain = *max_element(gains,gains+N);
        minGain -= 0.1 * (maxGain - minGain);
        maxGain += 0.1 * (maxGain - minGain);
    }
    else
    {
        minGain = get<1>(gainAxisScale);
        maxGain = get<2>(gainAxisScale);
    }

    if (get<0>(phaseAxisScale)) // autoscale is true
    {
        minPhase = *min_element(phases,phases+N);
        maxPhase = *max_element(phases,phases+N);
        minPhase -= 0.1 * (maxPhase - minPhase);
        maxPhase += 0.1 * (maxPhase - minPhase);
    }
    else
    {
        minPhase = get<1>(phaseAxisScale);
        maxPhase = get<2>(phaseAxisScale);
    }

    // Set up options for grids
    if (get<2>(freqAxisIntervals))
    {
        freqAxisOpts += "g";
    }
    if (get<3>(freqAxisIntervals))
    {
        freqAxisOpts += "h";
    }
    if (get<2>(gainAxisIntervals))
    {
        gainAxisOpts += "g";
    }
    if (get<3>(gainAxisIntervals))
    {
        gainAxisOpts += "h";
    }
    if (get<2>(phaseAxisIntervals))
    {
        phaseAxisOpts += "g";
    }
    if (get<3>(phaseAxisIntervals))
    {
        phaseAxisOpts += "h";
    }

    pladv( 0 );

    // Setup the viewport to take up +/- 15% horizontal and +/- 10% vertical
    plvpor( 0.15, 0.85, 0.1, 0.9 );

    // Specify the coordinates of the viewport boundaries for gain
    plwind( minFreq, maxFreq, minGain, maxGain );

    // Set the color of the viewport box
    plcol0( 0 );
    // Setup box and axes for gain
    plbox( freqAxisOpts.c_str(), get<0>(freqAxisIntervals), get<1>(freqAxisIntervals), 
           gainAxisOpts.c_str(), get<0>(gainAxisIntervals), get<1>(gainAxisIntervals) );
    // Set the color of the gain data
    plcol0( 9 );
    // Plot the gain data
    plline( N, freqs, gains );

    // Start Labeling
    plcol0( 0 );
    plmtex( "b", 3.2, 0.5, 0.5, "Frequency Log(Hz)" );
    plmtex( "t", 2.0, 0.5, 0.5, "Frequency Response Bode Plot" );
    plcol0( 9 );
    plmtex( "l", 5.0, 0.5, 0.5, "Gain (dB)" );

    // Specify the coordinates of the viewport boundaries for phase
    plwind( minFreq, maxFreq, minPhase, maxPhase );
    // Set the color of the viewport box
    plcol0( 0 );
    // Setup axes for phase
    plbox( "", 0.0, 0, phaseAxisOpts.c_str(), get<0>(phaseAxisIntervals), get<1>(phaseAxisIntervals) );
    // Set the color of the phase data
    plcol1(0.9); // Something near red
    // Plot the phase data
    plline( N, freqs, phases );

    // Label phase data
    plcol1(0.9); // Something near red
    plmtex( "r", 5.0, 0.5, 0.5, "Phase (degrees)" );

    // Close PLplot library
    plend();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FRAPlotter::GetScreenBitmapPlot32BppBGRA
//
// Purpose: This function implements/hides the transformation of the PLPlot buffer to a format 
//          compatible with the UI.  This could be mem (24bpp RGB), memcairo (32bpp RGBA) to 
//          WinGDI (32bpp BGRA) or something else.
//
// Parameters: [out] return: a unique pointer to the converted buffer
//
// Notes: Actual work is carried out by worker thread of this object.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

unique_ptr<uint8_t[]> FRAPlotter::GetScreenBitmapPlot32BppBGRA(void)
{
    DWORD dwWaitResult;
    unique_ptr<uint8_t[]> buffer;

    buffer = unique_ptr<uint8_t[]>(new uint8_t[plotWidth*plotHeight*4]);

    cmd = CMD_MAKE_SCREEN_BITMAP_32BPP_BGRA;
    cmdResult = indeterminate;
    workingBuffer = buffer.get();
    SetEvent(hFraPlotterExecuteCommandEvent);
    dwWaitResult = WaitForSingleObject( hFraPlotterCommandCompleteEvent, INFINITE );

    if (dwWaitResult == WAIT_OBJECT_0)
    {
        return move(buffer);
    }
    else
    {
        throw runtime_error("Application Error: Invalid wait result in GetScreenBitmapPlot32BppBGRA");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FRAPlotter::GetPNGBitmapPlot
//
// Purpose: Create a PNG file from the plot most recently generated.
//
// Parameters: [out] return: a unique pointer to the PNG buffer
//
// Notes:  Actual work is carried out by worker thread of this object.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

unique_ptr<uint8_t[]> FRAPlotter::GetPNGBitmapPlot(size_t* size)
{
    DWORD dwWaitResult;

    cmd = CMD_MAKE_FILE_BITMAP_PNG;
    cmdResult = indeterminate;
    SetEvent(hFraPlotterExecuteCommandEvent);
    dwWaitResult = WaitForSingleObject( hFraPlotterCommandCompleteEvent, INFINITE );

    if (dwWaitResult == WAIT_OBJECT_0)
    {
        if (cmdResult == true)
        {
            *size = workingBufferSize;
        }
        else
        {
            *size = 0;
        }
        unique_ptr<uint8_t[]> buffer(workingBuffer);
        return move(buffer);
    }
    else
    {
        throw runtime_error("Application Error: Invalid wait result in GetPNGBitmapPlot");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FRAPlotter::WorkerThread
//
// Purpose: A thread function performing the services of the object.
//
// Parameters: [in] lpThreadParameter - Parameter to pass to the thread on creation.  In this case
//                                      it's used to pass the object instance pointer.
//
// Notes: This function uses the Windows Imaging Component (WIC)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI FRAPlotter::WorkerThread(LPVOID lpThreadParameter)
{
    static bool initSucceeded = true;
    HRESULT hr;
    DWORD dwWaitResult;
    FRAPlotter* instance = (FRAPlotter*)lpThreadParameter;
    IWICImagingFactory     *pIWICFactory = NULL;
    IWICBitmap             *pPlotBitmap24RGB;

    // Initialize Com
    if(FAILED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
        initSucceeded = false;
    }
    // Create WIC factory
    if (FAILED(hr = CoCreateInstance( CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory))))
    {
        initSucceeded = false;
    }

    for (;;)
    {
        dwWaitResult = WaitForSingleObject( instance->hFraPlotterExecuteCommandEvent, INFINITE );

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            // If we don't have a valid WIC Factory, signal failure and reset;
            if (initSucceeded == false)
            {
                instance->cmdResult = false;
                SetEvent(instance->hFraPlotterCommandCompleteEvent);
                continue;
            }

            // Decode the command
            if (instance->cmd == CMD_MAKE_SCREEN_BITMAP_32BPP_BGRA)
            {
                IWICBitmapSource       *pPlotBitmap32BGRA;

                if (FAILED(hr = pIWICFactory -> CreateBitmapFromMemory( instance->plotWidth, instance->plotHeight, GUID_WICPixelFormat24bppRGB,
                                (instance->bytesPerPixel)*(instance->plotWidth),
                                (instance->plotWidth)*(instance->plotHeight)*(instance->bytesPerPixel),
                                instance->plotBmBuffer.data(), &pPlotBitmap24RGB)))
                {
                    instance->cmdResult = false;
                }
                // Convert to a GDI compatible format (32BPP BGRA)
                else if (FAILED(hr = WICConvertBitmapSource( GUID_WICPixelFormat32bppBGRA, pPlotBitmap24RGB, &pPlotBitmap32BGRA)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pPlotBitmap32BGRA->CopyPixels( 0, instance->plotWidth*4, (instance->plotWidth)*(instance->plotHeight)*4, instance->workingBuffer)))
                {
                    instance->cmdResult = false;
                }
                else
                {
                    instance->cmdResult = true;
                }

                SafeRelease(&pPlotBitmap24RGB);
                SafeRelease(&pPlotBitmap32BGRA);

                SetEvent(instance->hFraPlotterCommandCompleteEvent);
            }
            else if (instance->cmd == CMD_MAKE_FILE_BITMAP_PNG)
            {
                IStream* pIStream = NULL;
                IWICStream *pIWICStream = NULL;
                IWICBitmapEncoder *pEncoder = NULL;
                IWICBitmapFrameEncode *pFrameEncode = NULL;
                WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;

                if (FAILED(hr = CreateStreamOnHGlobal( NULL, TRUE, &pIStream )))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pIWICFactory->CreateStream(&pIWICStream)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pIWICStream->InitializeFromIStream(pIStream)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pIWICFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &pEncoder)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pEncoder->Initialize(pIWICStream, WICBitmapEncoderNoCache)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pEncoder->CreateNewFrame(&pFrameEncode, NULL)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pFrameEncode->Initialize(NULL)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pFrameEncode->SetSize(instance->plotWidth, instance->plotHeight)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pFrameEncode->SetPixelFormat(&format)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pIWICFactory -> CreateBitmapFromMemory( instance->plotWidth, instance->plotHeight, GUID_WICPixelFormat24bppRGB,
                                     (instance->bytesPerPixel)*(instance->plotWidth),
                                     (instance->plotWidth)*(instance->plotHeight)*(instance->bytesPerPixel),
                                     instance->plotBmBuffer.data(), &pPlotBitmap24RGB)))
                {
                    instance->cmdResult = false;
                }

                else if (FAILED(hr = pFrameEncode->WriteSource(pPlotBitmap24RGB, NULL)))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pFrameEncode->Commit()))
                {
                    instance->cmdResult = false;
                }
                else if (FAILED(hr = pEncoder->Commit()))
                {
                    instance->cmdResult = false;
                }
                else
                {
                    LARGE_INTEGER li = {0};
                    STATSTG statStg;
                    if (FAILED(hr = pIStream->Seek(li, STREAM_SEEK_SET, NULL)))
                    {
                        instance->cmdResult = false;
                    }
                    else if (FAILED(hr = pIStream->Stat( &statStg, STATFLAG_NONAME )))
                    {
                        instance->cmdResult = false;
                    }
                    else
                    {
                        instance->workingBuffer = new uint8_t[statStg.cbSize.LowPart];

                        if (FAILED(hr = pIStream->Read(instance->workingBuffer, statStg.cbSize.LowPart, NULL)))
                        {
                            instance->cmdResult = false;
                        }
                        else
                        {
                            instance->workingBufferSize = statStg.cbSize.LowPart;
                            instance->cmdResult = true;
                        }
                    }
                }

                SafeRelease(&pIStream);
                SafeRelease(&pIWICStream);
                SafeRelease(&pEncoder);
                SafeRelease(&pFrameEncode);
                SafeRelease(&pPlotBitmap24RGB);

                SetEvent(instance->hFraPlotterCommandCompleteEvent);
            }
            else if (instance->cmd == CMD_EXIT)
            {
                break;
            }
        }
        else
        {
            instance->cmdResult = false;
            SetEvent(instance->hFraPlotterCommandCompleteEvent);
        }
    }

    SafeRelease(&pIWICFactory);

    SetEvent(instance->hFraPlotterCommandCompleteEvent);

    return 0;
}
