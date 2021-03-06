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
// Module: PlotAxesDialog.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <Windowsx.h>
#include <string>
#include <sstream>
#include "utility.h"
#include "PlotAxesDialog.h"
#include "PicoScopeFraApp.h"
#include "Resource.h"

PlotScaleSettings_T* plotSettings;

// Strings to store initial values of the dialog edit controls to detect change.
wstring initialMinFreqStr, initialMaxFreqStr, initialFreqMajorIntervalStr, initialFreqTicksPerMajorStr, 
        initialMinGainStr, initialMaxGainStr, initialGainMajorIntervalStr, initialGainTicksPerMajorStr,
        initialMinPhaseStr, initialMaxPhaseStr, initialPhaseMajorIntervalStr, initialPhaseTicksPerMajorStr;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SettingsChanged
//
// Purpose: To detect if the settings in the plot axes dialog were changed
//
// Parameters: [in] hDlg: handle to the plot axes settings dialog
//
// Notes: Relies on the initial numeric information to be saved as strings for text comparison,
//        since comparing the floating point values would be inaccurate.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool SettingsChanged( HWND hDlg )
{
    HWND hndCtrl;    
    WCHAR editStr[32];
    bool freqAutoscale, freqMajorGrids, freqMinorGrids,
         gainAutoscale, gainMajorGrids, gainMinorGrids,
         phaseAutoscale, phaseMajorGrids, phaseMinorGrids,
         gainMasterGrids, phaseMasterGrids;
    wstring currentMinFreqStr, currentMaxFreqStr, currentFreqMajorIntervalStr, currentFreqTicksPerMajorStr, 
            currentMinGainStr, currentMaxGainStr, currentGainMajorIntervalStr, currentGainTicksPerMajorStr,
            currentMinPhaseStr, currentMaxPhaseStr, currentPhaseMajorIntervalStr, currentPhaseTicksPerMajorStr;
    
    // Get all the control states
    // Get freq axis settings
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_AUTOSCALE);
    freqAutoscale = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MIN);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentMinFreqStr = editStr;

    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAX);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentMaxFreqStr = editStr;

    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAJ_INT);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentFreqMajorIntervalStr = editStr;
    
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_TICKS_PER_MAJOR);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentFreqTicksPerMajorStr = editStr;
    
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAJOR_GRIDS);
    freqMajorGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MINOR_GRIDS);
    freqMinorGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    // Get gain axis settings
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_AUTOSCALE);
    gainAutoscale = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MIN);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentMinGainStr = editStr;

    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAX);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentMaxGainStr = editStr;

    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentGainMajorIntervalStr = editStr;
    
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentGainTicksPerMajorStr = editStr;
    
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
    gainMajorGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
    gainMinorGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MASTER_GRIDS);
    gainMasterGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    // Get phase axis settings
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_AUTOSCALE);
    phaseAutoscale = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MIN);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentMinPhaseStr = editStr;

    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAX);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentMaxPhaseStr = editStr;

    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentPhaseMajorIntervalStr = editStr;
    
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
    Edit_GetText( hndCtrl, editStr, sizeof(editStr)/sizeof(WCHAR) );
    currentPhaseTicksPerMajorStr = editStr;
    
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
    phaseMajorGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
    phaseMinorGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MASTER_GRIDS);
    phaseMasterGrids = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    return (currentMinFreqStr != initialMinFreqStr || currentMaxFreqStr != initialMaxFreqStr || currentFreqMajorIntervalStr != initialFreqMajorIntervalStr || currentFreqTicksPerMajorStr != initialFreqTicksPerMajorStr || 
            currentMinGainStr != initialMinGainStr || currentMaxGainStr != initialMaxGainStr || currentGainMajorIntervalStr != initialGainMajorIntervalStr || currentGainTicksPerMajorStr != initialGainTicksPerMajorStr || 
            currentMinPhaseStr != initialMinPhaseStr || currentMaxPhaseStr != initialMaxPhaseStr || currentPhaseMajorIntervalStr != initialPhaseMajorIntervalStr || currentPhaseTicksPerMajorStr != initialPhaseTicksPerMajorStr || 
            freqAutoscale != get<0>(plotSettings->freqAxisScale) || freqMajorGrids != get<2>(plotSettings->freqAxisIntervals) || freqMinorGrids != get<3>(plotSettings->freqAxisIntervals) || 
            gainAutoscale != get<0>(plotSettings->gainAxisScale) || gainMajorGrids != get<2>(plotSettings->gainAxisIntervals) || gainMinorGrids != get<3>(plotSettings->gainAxisIntervals) || gainMasterGrids != plotSettings->gainMasterIntervals ||
            phaseAutoscale != get<0>(plotSettings->phaseAxisScale) || phaseMajorGrids != get<2>(plotSettings->phaseAxisIntervals) || phaseMinorGrids != get<3>(plotSettings->phaseAxisIntervals) || phaseMasterGrids != plotSettings->phaseMasterIntervals );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ValidateAndStoreSettings
//
// Purpose: Checks plot settings for validity and stores them if they are valid
//
// Parameters: [in] hDlg: handle to the plot axes settings dialog
//
// Notes: N/A
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ValidateAndStoreSettings( HWND hDlg, PlotScaleSettings_T& dlgPlotSettings )
{
    bool retVal = true;
    WCHAR numberStr[32];
    HWND hndCtrl;
    wstring errorConditions[32];
    uint8_t numErrors = 0;
    bool minFreqValid = false;
    bool maxFreqValid = false;
    bool minGainValid = false;
    bool maxGainValid = false;
    bool minPhaseValid = false;
    bool maxPhaseValid = false;

    // Get freq axis settings
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_AUTOSCALE);
    get<0>(dlgPlotSettings.freqAxisScale) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MIN);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<1>(dlgPlotSettings.freqAxisScale) ))
    {
        errorConditions[numErrors++] = L"Frequency Minimum is not a valid number";
        retVal = false;
    }
    else
    {
        if (get<1>(dlgPlotSettings.freqAxisScale) <= 0.0)
        {
            errorConditions[numErrors++] = L"Frequency Minimum must be greater than 0";
            retVal = false;
        }
        else
        {
            minFreqValid = true;
        }
    }
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAX);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<2>(dlgPlotSettings.freqAxisScale) ))
    {
        errorConditions[numErrors++] = L"Frequency Maximum is not a valid number";
        retVal = false;
    }
    else
    {
        if (get<2>(dlgPlotSettings.freqAxisScale) <= 0.0)
        {
            errorConditions[numErrors++] = L"Frequency Maximum must be greater than 0";
            retVal = false;
        }
        else
        {
            maxFreqValid = true;
        }
    }
    
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAJOR_GRIDS);
    get<2>(dlgPlotSettings.freqAxisIntervals) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MINOR_GRIDS);
    get<3>(dlgPlotSettings.freqAxisIntervals) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    // Get gain axis settings
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_AUTOSCALE);
    get<0>(dlgPlotSettings.gainAxisScale) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MIN);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<1>(dlgPlotSettings.gainAxisScale) ))
    {
        errorConditions[numErrors++] = L"Gain Minimum is not a valid number";
        retVal = false;
    }
    else
    {
        minGainValid = true;
    }
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAX);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<2>(dlgPlotSettings.gainAxisScale) ))
    {
        errorConditions[numErrors++] = L"Gain Maximum is not a valid number";
        retVal = false;
    }
    else
    {
        maxGainValid = true;
    }
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<0>(dlgPlotSettings.gainAxisIntervals) ))
    {
        errorConditions[numErrors++] = L"Gain Major Interval is not a valid number";
        retVal = false;
    }
    else
    {
        // If the user entered a negative value, just correct it.
        get<0>(dlgPlotSettings.gainAxisIntervals) = fabs(get<0>(dlgPlotSettings.gainAxisIntervals));
    }
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint8( numberStr, get<1>(dlgPlotSettings.gainAxisIntervals) ))
    {
        errorConditions[numErrors++] = L"Gain Ticks/Major Interval must be a non-negative integer";
        retVal = false;
    }
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
    get<2>(dlgPlotSettings.gainAxisIntervals) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
    get<3>(dlgPlotSettings.gainAxisIntervals) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MASTER_GRIDS);
    dlgPlotSettings.gainMasterIntervals = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    // Get phase axis settings
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_AUTOSCALE);
    get<0>(dlgPlotSettings.phaseAxisScale) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MIN);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<1>(dlgPlotSettings.phaseAxisScale) ))
    {
        errorConditions[numErrors++] = L"Phase Minimum is not a valid number";
        retVal = false;
    }
    else
    {
        minPhaseValid = true;
    }
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAX);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<2>(dlgPlotSettings.phaseAxisScale) ))
    {
        errorConditions[numErrors++] = L"Phase Maximum is not a valid number";
        retVal = false;
    }
    else
    {
        maxPhaseValid = true;
    }
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToDouble( numberStr, get<0>(dlgPlotSettings.phaseAxisIntervals) ))
    {
        errorConditions[numErrors++] = L"Phase Major Interval is not a valid number";
        retVal = false;
    }
    else
    {
        // If the user entered a negative value, just correct it.
        get<0>(dlgPlotSettings.phaseAxisIntervals) = fabs(get<0>(dlgPlotSettings.phaseAxisIntervals));
    }
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint8( numberStr, get<1>(dlgPlotSettings.phaseAxisIntervals) ))
    {
        errorConditions[numErrors++] = L"Phase Ticks/Major Interval must be a non-negative integer";
        retVal = false;
    }
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
    get<2>(dlgPlotSettings.phaseAxisIntervals) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
    get<3>(dlgPlotSettings.phaseAxisIntervals) = (Button_GetCheck( hndCtrl ) == BST_CHECKED);
    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MASTER_GRIDS);
    dlgPlotSettings.phaseMasterIntervals = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    if (minFreqValid && maxFreqValid && get<1>(dlgPlotSettings.freqAxisScale) >= get<2>(dlgPlotSettings.freqAxisScale))
    {
        errorConditions[numErrors++] = L"Frequency Minimum must be less than Frequency Maximum";
        retVal = false;
    }
    if (minGainValid && maxGainValid && get<1>(dlgPlotSettings.gainAxisScale) >= get<2>(dlgPlotSettings.gainAxisScale))
    {
        errorConditions[numErrors++] = L"Gain Minimum must be less than Gain Maximum";
        retVal = false;
    }
    if (minPhaseValid && maxPhaseValid && get<1>(dlgPlotSettings.phaseAxisScale) >= get<2>(dlgPlotSettings.phaseAxisScale))
    {
        errorConditions[numErrors++] = L"Phase Minimum must be less than Phase Maximum";
        retVal = false;
    }

    if (!retVal)
    {
        uint8_t i;
        wstring errorMessage = L"The following are invalid:\n";
        for (i = 0; i < numErrors-1; i++)
        {
            errorMessage += L"- " + errorConditions[i] + L",\n";
        }
        errorMessage += L"- " + errorConditions[i];

        MessageBox( hDlg, errorMessage.c_str(), L"Error", MB_OK );
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PlotAxesDialogHandler
//
// Purpose: Dialog procedure for the Plot Axes Dialog.  Handles initialization and user actions.
//
// Parameters: See Windows API documentation
//
// Notes: N/A
//
///////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK PlotAxesDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hndCtrl;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            wstringstream wss;
            plotSettings = (PlotScaleSettings_T*)lParam;
            
            //// Populate the Dialog Box
            //// Populate Frequency Axis Values
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_AUTOSCALE);
            Button_SetCheck( hndCtrl, get<0>(plotSettings->freqAxisScale) ? BST_CHECKED : BST_UNCHECKED );
            
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MIN);
            wss.clear();
            wss.str(L"");
            wss << get<1>(plotSettings->freqAxisScale);
            initialMinFreqStr = wss.str();
            Edit_SetText( hndCtrl, initialMinFreqStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAX);
            wss.clear();
            wss.str(L"");
            wss << get<2>(plotSettings->freqAxisScale);
            initialMaxFreqStr = wss.str();
            Edit_SetText( hndCtrl, initialMaxFreqStr.c_str() );
            
            // Frequency axis intervals are currently ignored since we only implement log mode, so disable
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAJ_INT);
            Edit_Enable( hndCtrl, false );
            
            // Frequency axis intervals are currently ignored since we only implement log mode, so disable
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_TICKS_PER_MAJOR);
            Edit_Enable( hndCtrl, false );
            
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAJOR_GRIDS);
            Button_SetCheck( hndCtrl, get<2>(plotSettings->freqAxisIntervals) ? BST_CHECKED : BST_UNCHECKED );
            
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MINOR_GRIDS);
            Button_SetCheck( hndCtrl, get<3>(plotSettings->freqAxisIntervals) ? BST_CHECKED : BST_UNCHECKED );

            //// Populate Gain Axis Values
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_AUTOSCALE);
            Button_SetCheck( hndCtrl, get<0>(plotSettings->gainAxisScale) ? BST_CHECKED : BST_UNCHECKED );
            
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MIN);
            wss.clear();
            wss.str(L"");
            wss << get<1>(plotSettings->gainAxisScale);
            initialMinGainStr = wss.str();
            Edit_SetText( hndCtrl, initialMinGainStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAX);
            wss.clear();
            wss.str(L"");
            wss << get<2>(plotSettings->gainAxisScale);
            initialMaxGainStr = wss.str();
            Edit_SetText( hndCtrl, initialMaxGainStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
            wss.clear();
            wss.str(L"");
            wss << get<0>(plotSettings->gainAxisIntervals);
            initialGainMajorIntervalStr = wss.str();
            Edit_SetText( hndCtrl, initialGainMajorIntervalStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
            wss.clear();
            wss.str(L"");
            wss << get<1>(plotSettings->gainAxisIntervals);
            initialGainTicksPerMajorStr = wss.str();
            Edit_SetText( hndCtrl, initialGainTicksPerMajorStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
            Button_SetCheck( hndCtrl, get<2>(plotSettings->gainAxisIntervals) ? BST_CHECKED : BST_UNCHECKED );
            
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
            Button_SetCheck( hndCtrl, get<3>(plotSettings->gainAxisIntervals) ? BST_CHECKED : BST_UNCHECKED );

            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MASTER_GRIDS);
            Button_SetCheck( hndCtrl, plotSettings->gainMasterIntervals ? BST_CHECKED : BST_UNCHECKED );

            //// Populate Phase Axis Values
            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_AUTOSCALE);
            Button_SetCheck( hndCtrl, get<0>(plotSettings->phaseAxisScale) ? BST_CHECKED : BST_UNCHECKED );
            
            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MIN);
            wss.clear();
            wss.str(L"");
            wss << get<1>(plotSettings->phaseAxisScale);
            initialMinPhaseStr = wss.str();
            Edit_SetText( hndCtrl, initialMinPhaseStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAX);
            wss.clear();
            wss.str(L"");
            wss << get<2>(plotSettings->phaseAxisScale);
            initialMaxPhaseStr = wss.str();
            Edit_SetText( hndCtrl, initialMaxPhaseStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
            wss.clear();
            wss.str(L"");
            wss << get<0>(plotSettings->phaseAxisIntervals);
            initialPhaseMajorIntervalStr = wss.str();
            Edit_SetText( hndCtrl, initialPhaseMajorIntervalStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
            wss.clear();
            wss.str(L"");
            wss << get<1>(plotSettings->phaseAxisIntervals);
            initialPhaseTicksPerMajorStr = wss.str();
            Edit_SetText( hndCtrl, initialPhaseTicksPerMajorStr.c_str() );
            
            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
            Button_SetCheck( hndCtrl, get<2>(plotSettings->phaseAxisIntervals) ? BST_CHECKED : BST_UNCHECKED );
            
            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
            Button_SetCheck( hndCtrl, get<3>(plotSettings->phaseAxisIntervals) ? BST_CHECKED : BST_UNCHECKED );

            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MASTER_GRIDS);
            Button_SetCheck( hndCtrl, plotSettings->phaseMasterIntervals ? BST_CHECKED : BST_UNCHECKED );

            // Disable Min/Max edit fields if autoscaling is set.
            hndCtrl = GetDlgItem( hDlg, IDC_FREQ_AXIS_AUTOSCALE );
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MIN);
                Edit_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAX);
                Edit_Enable( hndCtrl, false );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MIN);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAX);
                Edit_Enable( hndCtrl, true );
            }

            hndCtrl = GetDlgItem( hDlg, IDC_GAIN_AXIS_AUTOSCALE );
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MIN);
                Edit_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAX);
                Edit_Enable( hndCtrl, false );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MIN);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAX);
                Edit_Enable( hndCtrl, true );
            }

            hndCtrl = GetDlgItem( hDlg, IDC_PHASE_AXIS_AUTOSCALE );
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MIN);
                Edit_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAX);
                Edit_Enable( hndCtrl, false );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MIN);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAX);
                Edit_Enable( hndCtrl, true );
            }

            // Disable minor grid settings if major grids are not on
            // Order matter here, this needs to come before code that disables interval settings for master grids
            hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAJOR_GRIDS);
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MINOR_GRIDS);
                Button_Enable( hndCtrl, true );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MINOR_GRIDS);
                Button_SetCheck( hndCtrl, BST_UNCHECKED );
                Button_Enable( hndCtrl, false );
            }
            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                Button_Enable( hndCtrl, true );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                Button_SetCheck( hndCtrl, BST_UNCHECKED );
                Button_Enable( hndCtrl, false );
            }

            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                Button_Enable( hndCtrl, true );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                Button_SetCheck( hndCtrl, BST_UNCHECKED );
                Button_Enable( hndCtrl, false );
            }

            // Disable interval settings if a master grids checkbox is checked
            hndCtrl = GetDlgItem( hDlg, IDC_GAIN_AXIS_MASTER_GRIDS );
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem( hDlg, IDC_PHASE_AXIS_MASTER_GRIDS );
                Button_SetCheck( hndCtrl, BST_UNCHECKED );
                        
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
                Button_Enable( hndCtrl, true );
                if (Button_GetCheck(hndCtrl))
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                    Button_Enable( hndCtrl, true );
                }
                else
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                    Button_SetCheck( hndCtrl, BST_UNCHECKED );
                    Button_Enable( hndCtrl, false );
                }

                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
                Edit_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
                Edit_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
                Button_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                Button_Enable( hndCtrl, false );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
                Button_Enable( hndCtrl, true );
                if (Button_GetCheck(hndCtrl))
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                    Button_Enable( hndCtrl, true );
                }
                else
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                    Button_SetCheck( hndCtrl, BST_UNCHECKED );
                    Button_Enable( hndCtrl, false );
                }
            }

            hndCtrl = GetDlgItem( hDlg, IDC_PHASE_AXIS_MASTER_GRIDS );
            if (Button_GetCheck(hndCtrl))
            {
                hndCtrl = GetDlgItem( hDlg, IDC_GAIN_AXIS_MASTER_GRIDS );
                Button_SetCheck( hndCtrl, BST_UNCHECKED );
                        
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
                Button_Enable( hndCtrl, true );
                if (Button_GetCheck(hndCtrl))
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                    Button_Enable( hndCtrl, true );
                }
                else
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                    Button_SetCheck( hndCtrl, BST_UNCHECKED );
                    Button_Enable( hndCtrl, false );
                }

                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
                Edit_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
                Edit_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
                Button_Enable( hndCtrl, false );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                Button_Enable( hndCtrl, false );
            }
            else
            {
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
                Edit_Enable( hndCtrl, true );
                hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
                Button_Enable( hndCtrl, true );
                if (Button_GetCheck(hndCtrl))
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                    Button_Enable( hndCtrl, true );
                }
                else
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                    Button_SetCheck( hndCtrl, BST_UNCHECKED );
                    Button_Enable( hndCtrl, false );
                }
            }

            return (INT_PTR)TRUE;
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_FREQ_AXIS_MAJOR_GRIDS:
                {
                    hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAJOR_GRIDS);
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MINOR_GRIDS);
                        Button_Enable( hndCtrl, true );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MINOR_GRIDS);
                        Button_SetCheck( hndCtrl, BST_UNCHECKED );
                        Button_Enable( hndCtrl, false );
                    }
                    return 0;
                    break;
                }
                case IDC_GAIN_AXIS_MAJOR_GRIDS:
                {
                    hndCtrl = GetDlgItem( hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS );
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                        Button_Enable( hndCtrl, true );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                        Button_SetCheck( hndCtrl, BST_UNCHECKED );
                        Button_Enable( hndCtrl, false );
                    }
                    return 0;
                    break;
                }
                case IDC_PHASE_AXIS_MAJOR_GRIDS:
                {
                    hndCtrl = GetDlgItem( hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS );
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                        Button_Enable( hndCtrl, true );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                        Button_SetCheck( hndCtrl, BST_UNCHECKED );
                        Button_Enable( hndCtrl, false );
                    }
                    return 0;
                    break;
                }
                case IDC_GAIN_AXIS_MASTER_GRIDS: // Gain Axis Master Grids checkbox was clicked
                {
                    hndCtrl = GetDlgItem( hDlg, IDC_GAIN_AXIS_MASTER_GRIDS );
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem( hDlg, IDC_PHASE_AXIS_MASTER_GRIDS );
                        Button_SetCheck( hndCtrl, BST_UNCHECKED );
                        
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
                        Button_Enable( hndCtrl, true );
                        if (Button_GetCheck(hndCtrl))
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                            Button_Enable( hndCtrl, true );
                        }
                        else
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                            Button_SetCheck( hndCtrl, BST_UNCHECKED );
                            Button_Enable( hndCtrl, false );
                        }

                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
                        Edit_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
                        Edit_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
                        Button_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                        Button_Enable( hndCtrl, false );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
                        Button_Enable( hndCtrl, true );
                        if (Button_GetCheck(hndCtrl))
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                            Button_Enable( hndCtrl, true );
                        }
                        else
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                            Button_SetCheck( hndCtrl, BST_UNCHECKED );
                            Button_Enable( hndCtrl, false );
                        }
                    }
                    return 0;
                    break;
                }
                case IDC_PHASE_AXIS_MASTER_GRIDS: // Phase Axis Master Grids checkbox was clicked
                {
                    hndCtrl = GetDlgItem( hDlg, IDC_PHASE_AXIS_MASTER_GRIDS );
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem( hDlg, IDC_GAIN_AXIS_MASTER_GRIDS );
                        Button_SetCheck( hndCtrl, BST_UNCHECKED );
                        
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJ_INT);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_TICKS_PER_MAJOR);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAJOR_GRIDS);
                        Button_Enable( hndCtrl, true );
                        if (Button_GetCheck(hndCtrl))
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                            Button_Enable( hndCtrl, true );
                        }
                        else
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MINOR_GRIDS);
                            Button_SetCheck( hndCtrl, BST_UNCHECKED );
                            Button_Enable( hndCtrl, false );
                        }

                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
                        Edit_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
                        Edit_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
                        Button_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                        Button_Enable( hndCtrl, false );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJ_INT);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_TICKS_PER_MAJOR);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAJOR_GRIDS);
                        Button_Enable( hndCtrl, true );
                        if (Button_GetCheck(hndCtrl))
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                            Button_Enable( hndCtrl, true );
                        }
                        else
                        {
                            hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MINOR_GRIDS);
                            Button_SetCheck( hndCtrl, BST_UNCHECKED );
                            Button_Enable( hndCtrl, false );
                        }
                    }
                    return 0;
                    break;
                }
                case IDC_FREQ_AXIS_AUTOSCALE:
                {
                    hndCtrl = GetDlgItem( hDlg, IDC_FREQ_AXIS_AUTOSCALE );
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MIN);
                        Edit_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAX);
                        Edit_Enable( hndCtrl, false );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MIN);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_FREQ_AXIS_MAX);
                        Edit_Enable( hndCtrl, true );
                    }
                    return 0;
                    break;
                }
                case IDC_GAIN_AXIS_AUTOSCALE:
                {
                    hndCtrl = GetDlgItem( hDlg, IDC_GAIN_AXIS_AUTOSCALE );
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MIN);
                        Edit_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAX);
                        Edit_Enable( hndCtrl, false );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MIN);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_GAIN_AXIS_MAX);
                        Edit_Enable( hndCtrl, true );
                    }
                    return 0;
                    break;
                }
                case IDC_PHASE_AXIS_AUTOSCALE:
                {
                    hndCtrl = GetDlgItem( hDlg, IDC_PHASE_AXIS_AUTOSCALE );
                    if (Button_GetCheck(hndCtrl))
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MIN);
                        Edit_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAX);
                        Edit_Enable( hndCtrl, false );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MIN);
                        Edit_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hDlg, IDC_PHASE_AXIS_MAX);
                        Edit_Enable( hndCtrl, true );
                    }
                    return 0;
                    break;
                }
                case IDC_STORE:
                {
                    PlotScaleSettings_T dlgPlotSettings;
                    if (ValidateAndStoreSettings(hDlg, dlgPlotSettings))
                    {
                        pSettings->SetFreqScale(dlgPlotSettings.freqAxisScale);
                        pSettings->SetGainScale(dlgPlotSettings.gainAxisScale);
                        pSettings->SetPhaseScale(dlgPlotSettings.phaseAxisScale);

                        pSettings->SetFreqIntervals(dlgPlotSettings.freqAxisIntervals);
                        pSettings->SetGainIntervals(dlgPlotSettings.gainAxisIntervals);
                        pSettings->SetPhaseIntervals(dlgPlotSettings.phaseAxisIntervals);

                        pSettings->SetGainMasterIntervals(dlgPlotSettings.gainMasterIntervals);
                        pSettings->SetPhaseMasterIntervals(dlgPlotSettings.phaseMasterIntervals);
                    }
                    return (INT_PTR)TRUE;
                }
                case IDOK:
                {
                    PlotScaleSettings_T dlgPlotSettings;
                    if (SettingsChanged(hDlg))
                    {
                        if (ValidateAndStoreSettings(hDlg, dlgPlotSettings))
                        {
                            *plotSettings = dlgPlotSettings;
                            EndDialog(hDlg, IDOK);
                        }
                    }
                    else
                    {
                        EndDialog(hDlg, IDCANCEL);
                    }
                    return (INT_PTR)TRUE;
                }
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
            }
            return (INT_PTR)TRUE;
            break;
        }
        
        default:
            return (INT_PTR)FALSE;
    }
}