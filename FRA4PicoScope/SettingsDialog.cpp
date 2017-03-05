//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2014, 2015 by Aaron Hexamer
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
// Module: SettingsDialog.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

// Suppress an unnecessary C4996 warning about use of checked
// iterators invoked from Boost
#if !defined(_SCL_SECURE_NO_WARNINGS)
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <Windowsx.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "utility.h"
#include "SettingsDialog.h"
#include "PlotAxesDialog.h"
#include "PicoScopeFraApp.h"
#include "PicoScopeInterface.h"
#include "Resource.h"

const uint8_t numLogVerbosityFlags = 11;

const wchar_t logVerbosityString[numLogVerbosityFlags][128] =
{
    L"Scope Access Diagnostics",
    L"FRA Progress",
    L"Step Trial Progress",
    L"Signal Generator Diagnostics",
    L"Autorange Diagnostics",
    L"Adaptive Stimulus Diagnostics",
    L"Sample Processing Diagnostics",
    L"DFT Diagnostics",
    L"Scope Power Events",
    L"Save/Export Status",
    L"FRA Warnings"
};

bool logVerbositySelectorOpen = false;
PicoScope* pCurrentScope = NULL;

std::wstring PrintSampleRate(double sampleRate)
{
    std::wstring units;
    std::wstringstream valueSS;
    std::wstring valueStr;

    if (sampleRate >= 1.0e9)
    {
        sampleRate /= 1.0e9;
        units = L" GHz";
    }
    else if (sampleRate >= 1.0e6)
    {
        sampleRate /= 1.0e6;
        units = L" MHz";
    }
    else if (sampleRate >= 1.0e3)
    {
        sampleRate /= 1.0e3;
        units = L" kHz";
    }
    else
    {
        units = L" Hz";
    }

    // Output using fixed precision
    valueSS.precision(3);
    valueSS << std::fixed << sampleRate;
    valueStr = valueSS.str();

    // Smash trailing zeros right of a decimal point
    if (string::npos != valueStr.find(L"."))
    {
        boost::algorithm::trim_right_if( valueStr, boost::algorithm::is_any_of(L"0") );
        // If there's a decimal point remaining on the end, it needs to be stripped too
        boost::algorithm::trim_right_if( valueStr, boost::algorithm::is_any_of(L".") );
    }
    // Finally, correct for possibility of "-0"
    if (0 == valueStr.compare(L"-0"))
    {
        valueStr = L"0";
    }

    valueStr += units;

    return (valueStr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ValidateAndStoreSettings
//
// Purpose: Checks settings for validity and stores them if they are valid
//
// Parameters: [in] hDlg: handle to the settings dialog
//
// Notes: N/A
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ValidateAndStoreSettings( HWND hDlg )
{
    bool retVal = true;
    WCHAR numberStr[32];
    HWND hndCtrl;
    wstring errorConditions[32];
    uint8_t numErrors = 0;

    SamplingMode_T sampleMode = LOW_NOISE;
    bool sweepDescending = false;
    uint16_t extraSettlingTime = 0;
    bool adaptiveStimulusMode = false;
    uint8_t adaptiveStimulusTriesPerStep = 0;
    double adaptiveStimulusTargetTolerance = 0.0;
    wstring adaptiveStimulusTargetToleranceStr;
    bool qualityLimitsEnable = false;
    double amplitudeLowerLimit = 0.0;
    wstring amplitudeLowerLimitStr;
    double purityLowerLimit = 0.0;
    wstring purityLowerLimitStr;
    bool excludeDcFromNoise = false;
    int32_t inputStartRange = 0;
    int32_t outputStartRange = 0;
    uint8_t autorangeTriesPerStep = 0;
    double autorangeTolerance = 0.0;
    wstring autorangeToleranceStr;
    uint16_t minCyclesCaptured = 0;
    uint16_t maxCyclesCaptured = 0;
    uint16_t lowNoiseOversampling = 0;
    double noiseRejectModeBandwidth = 0.0;
    wstring noiseRejectModeBandwidthStr;
    uint32_t noiseRejectModeTimebase = 0;
    double phaseWrappingThreshold = 0.0;
    wstring phaseWrappingThresholdStr;
    double gainMarginPhaseCrossover = 0.0;
    wstring gainMarginPhaseCrossoverStr;
    bool timeDomainDiagnosticPlots = false;
    uint16_t logVerbosityFlags = 0;

    int curSel;
    bool minCyclesValid = false;
    bool maxCyclesValid = false;

    hndCtrl = GetDlgItem( hDlg, IDC_RADIO_NOISE_REJECT_MODE );
    if (Button_GetCheck( hndCtrl ) ==  BST_CHECKED)
    {
        sampleMode = HIGH_NOISE;
    }
    else
    {
        sampleMode = LOW_NOISE;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_RADIO_SWEEP_DESCENDING );
    sweepDescending = (Button_GetCheck( hndCtrl ) ==  BST_CHECKED);

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_EXTRA_SETTLING_TIME );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint16( numberStr, extraSettlingTime ))
    {
        errorConditions[numErrors++] = L"Extra settling time is not a valid number";
        retVal = false;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_ADAPTIVE_STIMULUS_ENABLE );
    adaptiveStimulusMode = (Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_ADAPTIVE_STIMULUS_TRIES_PER_STEP );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint8( numberStr, adaptiveStimulusTriesPerStep ))
    {
        errorConditions[numErrors++] = L"Adaptive stimulus tries/step is not a valid number";
        retVal = false;
    }
    else if (adaptiveStimulusTriesPerStep < 1)
    {
        errorConditions[numErrors++] = L"Adaptive stimulus tries/step must be >= 1";
        retVal = false;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_ADAPTIVE_STIMULUS_RESPONSE_TARGET_TOLERANCE );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    adaptiveStimulusTargetToleranceStr = numberStr;
    if (!WStringToDouble( adaptiveStimulusTargetToleranceStr, adaptiveStimulusTargetTolerance ))
    {
        errorConditions[numErrors++] = L"Adaptive stimulus target tolerance is not a valid number";
        retVal = false;
    }
    // Allow negative values because they can have meaning

    // Autorange Settings
    if (pCurrentScope)
    {
        hndCtrl = GetDlgItem( hDlg, IDC_COMBO_INPUT_START_RANGE );
        if (CB_ERR != (curSel = ComboBox_GetCurSel(hndCtrl)))
        {
            inputStartRange = ComboBox_GetItemData(hndCtrl, curSel);
        }
        else
        {
            errorConditions[numErrors++] = L"Input start range selection is invalid";
            retVal = false;
        }

        hndCtrl = GetDlgItem( hDlg, IDC_COMBO_OUTPUT_START_RANGE );
        if (CB_ERR != (curSel = ComboBox_GetCurSel(hndCtrl)))
        {
            outputStartRange = ComboBox_GetItemData(hndCtrl, curSel);
        }
        else
        {
            errorConditions[numErrors++] = L"Output start range selection is invalid";
            retVal = false;
        }
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_AUTORANGE_TRIES_PER_STEP );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint8( numberStr, autorangeTriesPerStep ))
    {
        errorConditions[numErrors++] = L"Autorange tries/step is not a valid number";
        retVal = false;
    }
    else if (autorangeTriesPerStep < 1)
    {
        errorConditions[numErrors++] = L"Autorange tries/step must be >= 1";
        retVal = false;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_AUTORANGE_TOLERANCE );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    autorangeToleranceStr = numberStr;
    if (!WStringToDouble( autorangeToleranceStr, autorangeTolerance ))
    {
        errorConditions[numErrors++] = L"Autorange tolerance is not a valid number";
        retVal = false;
    }
    else  if (autorangeTolerance <= 0.0)
    {
        errorConditions[numErrors++] = L"Autorange tolerance must be > 0.0";
        retVal = false;
    }

    // FRA Sample Settings
    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_MINIMUM_CYCLES_CAPTURED );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint16( numberStr, minCyclesCaptured ))
    {
        errorConditions[numErrors++] = L"Minimum cycles captured is not a valid number";
        retVal = false;
    }
    else if (minCyclesCaptured < 1)
    {
        errorConditions[numErrors++] = L"Minimum cycles captured must be >= 1";
        retVal = false;
    }
    else
    {
        minCyclesValid = true;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_MAXIMUM_CYCLES_CAPTURED );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint16( numberStr, maxCyclesCaptured ))
    {
        errorConditions[numErrors++] = L"Maximum cycles captured is not a valid number";
        retVal = false;
    }
    else if (maxCyclesCaptured < 1)
    {
        errorConditions[numErrors++] = L"Maximum cycles captured must be >= 1";
        retVal = false;
    }
    else
    {
        maxCyclesValid = true;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_LOW_NOISE_OVERSAMPLING );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    if (!WStringToUint16( numberStr, lowNoiseOversampling ))
    {
        errorConditions[numErrors++] = L"Low noise oversampling is not a valid number";
        retVal = false;
    }
    else if (lowNoiseOversampling < 2)
    {
        errorConditions[numErrors++] = L"Low noise oversampling must be >= 2";
        retVal = false;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_NOISE_REJECT_BW );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    noiseRejectModeBandwidthStr = numberStr;
    if (!WStringToDouble( noiseRejectModeBandwidthStr, noiseRejectModeBandwidth ))
    {
        errorConditions[numErrors++] = L"Noise reject bandwidth is not a valid number";
        retVal = false;
    }
    else if (noiseRejectModeBandwidth <= 0.0)
    {
        errorConditions[numErrors++] = L"Noise reject bandwidth must be > 0.0";
        retVal = false;
    }

    if (pCurrentScope)
    {
        hndCtrl = GetDlgItem( hDlg, IDC_EDIT_NOISE_REJECT_TIMEBASE );
        Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
        if (!WStringToUint32( numberStr, noiseRejectModeTimebase ))
        {
            errorConditions[numErrors++] = L"Noise reject timebase is not a valid number";
            retVal = false;
        }
        else if (noiseRejectModeTimebase < pCurrentScope->GetMinTimebase() || noiseRejectModeTimebase > pCurrentScope->GetMaxTimebase())
        {
            errorConditions[numErrors++] = L"Noise reject timebase is not valid";
            retVal = false;
        }
    }

    // Quality Limits
    hndCtrl = GetDlgItem( hDlg, IDC_QUALITY_LIMITS_ENABLE );
    qualityLimitsEnable = (Button_GetCheck(hndCtrl) == BST_CHECKED);

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_AMPLITUDE_LOWER_QUALITY_LIMIT );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    amplitudeLowerLimitStr = numberStr;
    if (!WStringToDouble( amplitudeLowerLimitStr, amplitudeLowerLimit ))
    {
        errorConditions[numErrors++] = L"Amplitude lower quality limit is not a valid number";
        retVal = false;
    }
    else if (amplitudeLowerLimit < 0.0)
    {
        errorConditions[numErrors++] = L"Amplitude lower quality limit must be >= 0.0";
        retVal = false;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_PURITY_LOWER_QUALITY_LIMIT );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    purityLowerLimitStr = numberStr;
    if (!WStringToDouble( purityLowerLimitStr, purityLowerLimit ))
    {
        errorConditions[numErrors++] = L"Purity lower quality limit is not a valid number";
        retVal = false;
    }
    else if (purityLowerLimit < 0.0)
    {
        errorConditions[numErrors++] = L"Purity lower quality limit must be >= 0.0";
        retVal = false;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EXCLUDE_DC_FROM_NOISE );
    excludeDcFromNoise = (Button_GetCheck(hndCtrl) == BST_CHECKED );

    // FRA Bode Plot Options
    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_PHASE_WRAPPING_THRESHOLD );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    phaseWrappingThresholdStr = numberStr;
    if (!WStringToDouble( phaseWrappingThresholdStr, phaseWrappingThreshold ))
    {
        errorConditions[numErrors++] = L"Phase wrapping threshold is not a valid number";
        retVal = false;
    }

    hndCtrl = GetDlgItem( hDlg, IDC_EDIT_GAIN_MARGIN_PHASE_CROSSOVER );
    Edit_GetText( hndCtrl, numberStr, sizeof(numberStr)/sizeof(WCHAR) );
    gainMarginPhaseCrossoverStr = numberStr;
    if (!WStringToDouble( gainMarginPhaseCrossoverStr, gainMarginPhaseCrossover ))
    {
        errorConditions[numErrors++] = L"Gain margin phase crossover is not a valid number";
        retVal = false;
    }

    if (minCyclesValid && maxCyclesValid && minCyclesCaptured > maxCyclesCaptured)
    {
        errorConditions[numErrors++] = L"Minimum cycles captured must be <= maximum cycles captured";
        retVal = false;
    }

    // Diagnostic settings
    hndCtrl = GetDlgItem( hDlg, IDC_TIME_DOMAIN_DIAGNOSTIC_PLOTS_ENABLE );
    timeDomainDiagnosticPlots = (Button_GetCheck(hndCtrl) == BST_CHECKED);

    hndCtrl = GetDlgItem( hDlg, IDC_LIST_LOG_VERBOSITY );
    for (int i = 0; i < numLogVerbosityFlags; i++)
    {
        if (ListView_GetCheckState(hndCtrl, i) != 0)
        {
            logVerbosityFlags |= (1 << i);
        }
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
    else
    {
        pSettings->SetSamplingMode(sampleMode);
        pSettings->SetSweepDescending(sweepDescending);
        pSettings->SetExtraSettlingTimeMs(extraSettlingTime);
        pSettings->SetAdaptiveStimulusMode(adaptiveStimulusMode);
        pSettings->SetAdaptiveStimulusTriesPerStep(adaptiveStimulusTriesPerStep);
        pSettings->SetTargetResponseAmplitudeTolerance(adaptiveStimulusTargetToleranceStr);
        pSettings->SetInputStartingRange(inputStartRange);
        pSettings->SetOutputStartingRange(outputStartRange);
        pSettings->SetAutorangeTriesPerStep(autorangeTriesPerStep);
        pSettings->SetAutorangeTolerance(autorangeToleranceStr);
        pSettings->SetMinCyclesCaptured(minCyclesCaptured);
        pSettings->SetMaxCyclesCaptured(maxCyclesCaptured);
        pSettings->SetLowNoiseOversampling(lowNoiseOversampling);
        pSettings->SetNoiseRejectBandwidth(noiseRejectModeBandwidthStr);
        pSettings->SetNoiseRejectModeTimebase(noiseRejectModeTimebase);
        pSettings->SetQualityLimitsState(qualityLimitsEnable);
        pSettings->SetAmplitudeLowerLimit(amplitudeLowerLimitStr);
        pSettings->SetPurityLowerLimit(purityLowerLimitStr);
        pSettings->SetDcExcludedFromNoiseState(excludeDcFromNoise);
        pSettings->SetPhaseWrappingThreshold(phaseWrappingThresholdStr);
        pSettings->SetGainMarginPhaseCrossover(gainMarginPhaseCrossoverStr);
        pSettings->SetTimeDomainPlotsEnabled(timeDomainDiagnosticPlots);
        pSettings->SetLogVerbosityFlags(logVerbosityFlags);
    }

    return retVal;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SettingsDialogHandler
//
// Purpose: Dialog procedure for the Settings Dialog.  Handles initialization and user actions.
//
// Parameters: See Windows API documentation
//
// Notes: N/A
//
///////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK SettingsDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hndCtrl;
    switch (message)
    {
        case WM_INITDIALOG:
        {
            LVCOLUMN listViewCol;
            LVITEM listViewItem;

            pCurrentScope = (PicoScope*)lParam;

            // FRA Execution Options
            if (HIGH_NOISE == pSettings->GetSamplingMode())
            {
                hndCtrl = GetDlgItem( hDlg, IDC_RADIO_NOISE_REJECT_MODE );
                Button_SetCheck( hndCtrl, BST_CHECKED );
            }
            else
            {
                hndCtrl = GetDlgItem( hDlg, IDC_RADIO_LOW_NOISE_MODE );
                Button_SetCheck( hndCtrl, BST_CHECKED );
            }

            if (pSettings->GetSweepDescending())
            {
                hndCtrl = GetDlgItem( hDlg, IDC_RADIO_SWEEP_DESCENDING );
                Button_SetCheck( hndCtrl, BST_CHECKED );
            }
            else
            {
                hndCtrl = GetDlgItem( hDlg, IDC_RADIO_SWEEP_ASCENDING );
                Button_SetCheck( hndCtrl, BST_CHECKED );
            }

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_EXTRA_SETTLING_TIME );
            Edit_SetText( hndCtrl, pSettings->GetExtraSettlingTimeMsAsString().c_str() );

            // Adaptive Stimulus
            hndCtrl = GetDlgItem( hDlg, IDC_ADAPTIVE_STIMULUS_ENABLE );
            Button_SetCheck( hndCtrl, pSettings->GetAdaptiveStimulusMode() ? BST_CHECKED : BST_UNCHECKED );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_ADAPTIVE_STIMULUS_TRIES_PER_STEP );
            Edit_SetText( hndCtrl, pSettings->GetAdaptiveStimulusTriesPerStepAsString().c_str() );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_ADAPTIVE_STIMULUS_RESPONSE_TARGET_TOLERANCE );
            Edit_SetText( hndCtrl, pSettings->GetTargetResponseAmplitudeToleranceAsString().c_str() );

            // Autorange Settings
            if (pCurrentScope)
            {
                int inputMinRange, inputMaxRange, outputMinRange, outputMaxRange;
                int currentStartingRange;
                const RANGE_INFO_T* rangeInfo = pCurrentScope->GetRangeCaps();
                inputMinRange = pCurrentScope->GetMinRange((PS_COUPLING)(pSettings->GetInputCoupling()));
                inputMaxRange = pCurrentScope->GetMaxRange((PS_COUPLING)(pSettings->GetInputCoupling()));
                outputMinRange = pCurrentScope->GetMinRange((PS_COUPLING)(pSettings->GetOutputCoupling()));
                outputMaxRange = pCurrentScope->GetMaxRange((PS_COUPLING)(pSettings->GetOutputCoupling()));

                hndCtrl = GetDlgItem( hDlg, IDC_COMBO_INPUT_START_RANGE );
                ComboBox_AddString( hndCtrl, L"Stimulus" );
                ComboBox_SetItemData( hndCtrl, 0, -1 );
                for (int rangeIndex = inputMinRange; rangeIndex <= inputMaxRange; rangeIndex++)
                {
                    ComboBox_SetItemData( hndCtrl, ComboBox_AddString( hndCtrl, rangeInfo[rangeIndex].name ), rangeIndex );
                }

                currentStartingRange = pSettings->GetInputStartingRange();
                if (-1 == currentStartingRange)
                {
                    ComboBox_SetCurSel(hndCtrl, 0);
                }
                else
                {
                    ComboBox_SetCurSel(hndCtrl, currentStartingRange - inputMinRange + 1);
                }

                hndCtrl = GetDlgItem( hDlg, IDC_COMBO_OUTPUT_START_RANGE );
                ComboBox_AddString( hndCtrl, L"Stimulus" );
                ComboBox_SetItemData( hndCtrl, 0, -1 );
                for (int rangeIndex = inputMinRange; rangeIndex <= inputMaxRange; rangeIndex++)
                {
                    ComboBox_SetItemData( hndCtrl, ComboBox_AddString( hndCtrl, rangeInfo[rangeIndex].name ), rangeIndex );
                }

                currentStartingRange = pSettings->GetOutputStartingRange();
                if (-1 == currentStartingRange)
                {
                    ComboBox_SetCurSel(hndCtrl, 0);
                }
                else
                {
                    ComboBox_SetCurSel(hndCtrl, currentStartingRange - outputMinRange + 1);
                }
            }
            else
            {
                hndCtrl = GetDlgItem( hDlg, IDC_COMBO_INPUT_START_RANGE );
                EnableWindow( hndCtrl, FALSE );

                hndCtrl = GetDlgItem( hDlg, IDC_STATIC_INPUT_START_RANGE );
                EnableWindow( hndCtrl, FALSE );

                hndCtrl = GetDlgItem( hDlg, IDC_COMBO_OUTPUT_START_RANGE );
                EnableWindow( hndCtrl, FALSE );

                hndCtrl = GetDlgItem( hDlg, IDC_STATIC_OUTPUT_START_RANGE );
                EnableWindow( hndCtrl, FALSE );
            }

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_AUTORANGE_TRIES_PER_STEP );
            Edit_SetText( hndCtrl, pSettings->GetAutorangeTriesPerStepAsString().c_str() );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_AUTORANGE_TOLERANCE );
            Edit_SetText( hndCtrl, pSettings->GetAutorangeToleranceAsString().c_str() );

            // FRA Sample Settings
            // Until issue 63 is implemented, we'll use min/max cycles and not bandwidth
            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_MINIMUM_CYCLES_CAPTURED );
            Edit_SetText( hndCtrl, pSettings->GetMinCyclesCapturedAsString().c_str() );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_MAXIMUM_CYCLES_CAPTURED );
            Edit_SetText( hndCtrl, pSettings->GetMaxCyclesCapturedAsString().c_str() );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_LOW_NOISE_OVERSAMPLING );
            Edit_SetText( hndCtrl, pSettings->GetLowNoiseOversamplingAsString().c_str() );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_FRA_NOISE_REJECT_BW );
            Edit_SetText( hndCtrl, pSettings->GetNoiseRejectBandwidthAsString().c_str() );
            EnableWindow( hndCtrl, FALSE );

            hndCtrl = GetDlgItem( hDlg, IDC_STATIC_FRA_NOISE_REJECT_BW );
            EnableWindow( hndCtrl, FALSE );

            if (pCurrentScope)
            {
                hndCtrl = GetDlgItem( hDlg, IDC_UPDOWN_NOISE_REJECT_TIMEBASE );
                SendMessage( hndCtrl, UDM_SETBUDDY, (WPARAM)GetDlgItem( hDlg, IDC_EDIT_NOISE_REJECT_TIMEBASE ), 0L );
                // Up/Down (Spin) controls with buddies only support signed 32 bit range.
                // That OK because it's contradictory to have very large timebases in noise reject mode anyway.
                SendMessage( hndCtrl, UDM_SETRANGE32, pCurrentScope->GetMinTimebase(), min((uint32_t)(std::numeric_limits<int32_t>::max)(), pCurrentScope->GetMaxTimebase()) );
                SendMessage( hndCtrl, UDM_SETPOS32, 0L, pSettings->GetNoiseRejectModeTimebaseAsInt() );
            }
            else
            {
                hndCtrl = GetDlgItem( hDlg, IDC_EDIT_NOISE_REJECT_TIMEBASE );
                EnableWindow( hndCtrl, FALSE );
                hndCtrl = GetDlgItem( hDlg, IDC_STATIC_NOISE_REJECT_TIMEBASE );
                EnableWindow( hndCtrl, FALSE );
                hndCtrl = GetDlgItem( hDlg, IDC_UPDOWN_NOISE_REJECT_TIMEBASE );
                EnableWindow( hndCtrl, FALSE );
                hndCtrl = GetDlgItem( hDlg, IDC_STATIC_NOISE_REJECT_SAMPLING_RATE );
                EnableWindow( hndCtrl, FALSE );
            }

            // Scope Specific Settings (base on currently selected scope)

            // Quality Limits
            hndCtrl = GetDlgItem( hDlg, IDC_QUALITY_LIMITS_ENABLE );
            Button_SetCheck( hndCtrl, pSettings->GetQualityLimitsState() ? BST_CHECKED : BST_UNCHECKED );
            EnableWindow( hndCtrl, FALSE );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_AMPLITUDE_LOWER_QUALITY_LIMIT );
            Edit_SetText( hndCtrl, pSettings->GetAmplitudeLowerLimitAsString().c_str() );
            EnableWindow( hndCtrl, FALSE );

            hndCtrl = GetDlgItem( hDlg, IDC_STATIC_AMPLITUDE_LOWER_QUALITY_LIMIT );
            EnableWindow( hndCtrl, FALSE );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_PURITY_LOWER_QUALITY_LIMIT );
            Edit_SetText( hndCtrl, pSettings->GetPurityLowerLimitAsString().c_str() );
            EnableWindow( hndCtrl, FALSE );

            hndCtrl = GetDlgItem( hDlg, IDC_STATIC_PURITY_LOWER_QUALITY_LIMIT );
            EnableWindow( hndCtrl, FALSE );

            hndCtrl = GetDlgItem( hDlg, IDC_EXCLUDE_DC_FROM_NOISE );
            Button_SetCheck( hndCtrl, pSettings->GetDcExcludedFromNoiseState() ? BST_CHECKED : BST_UNCHECKED );
            EnableWindow( hndCtrl, FALSE );

            // FRA Bode Plot Options
            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_PHASE_WRAPPING_THRESHOLD );
            Edit_SetText( hndCtrl, pSettings->GetPhaseWrappingThresholdAsString().c_str() );

            hndCtrl = GetDlgItem( hDlg, IDC_EDIT_GAIN_MARGIN_PHASE_CROSSOVER );
            Edit_SetText( hndCtrl, pSettings->GetGainMarginPhaseCrossoverAsString().c_str() );

            // Diagnostic settings
            hndCtrl = GetDlgItem( hDlg, IDC_TIME_DOMAIN_DIAGNOSTIC_PLOTS_ENABLE );
            Button_SetCheck( hndCtrl, pSettings->GetTimeDomainPlotsEnabled() ? BST_CHECKED : BST_UNCHECKED );

            hndCtrl = GetDlgItem( hDlg, IDC_LIST_LOG_VERBOSITY );
            ListView_SetExtendedListViewStyleEx( hndCtrl, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES );

            listViewCol.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
            listViewCol.pszText = L"";
            listViewCol.fmt = LVCFMT_LEFT;
            listViewCol.cx = 100; // Initial value does not matter because we're going to auto-size
            listViewCol.iSubItem = 0;
            ListView_InsertColumn( hndCtrl, 0, &listViewCol );

            listViewItem.mask = LVIF_TEXT;
            listViewItem.iSubItem = 0;
            for (int i = 0; i < numLogVerbosityFlags; i++)
            {
                listViewItem.iItem = i;
                listViewItem.pszText = (LPWSTR)logVerbosityString[i];
                ListView_InsertItem( hndCtrl, &listViewItem );
                if (pSettings->GetLogVerbosityFlag((LOG_MESSAGE_FLAGS_T)(1 << i)))
                {
                    ListView_SetCheckState(hndCtrl, i, TRUE);
                }
            }

            ListView_SetColumnWidth( hndCtrl, 0, LVSCW_AUTOSIZE );

            // Set initial state to closed
            SetWindowPos( hndCtrl, 0, 0, 0, LOWORD(ListView_ApproximateViewRect( hndCtrl, -1, -1, numLogVerbosityFlags )), 0, SWP_NOZORDER | SWP_NOMOVE);
            logVerbositySelectorOpen = false;

            return (INT_PTR)TRUE;
            break;
        }
        case WM_NOTIFY:
        {
            // Don't let items be selected in the log verbosity list, and toggle check when the item is clicked (even outside the checkbox)
            NMHDR* pNMHDR = ((NMHDR*)lParam);
            if (IDC_LIST_LOG_VERBOSITY == pNMHDR->idFrom &&
                LVN_ITEMCHANGED == pNMHDR->code)
            {
                NMLISTVIEW* pNMLV = ((NMLISTVIEW*)lParam);
                if (((pNMLV->uChanged & LVIF_STATE) == LVIF_STATE) && ((pNMLV->uNewState & LVIS_SELECTED) == LVIS_SELECTED))
                {
                    ListView_SetItemState(pNMHDR->hwndFrom, pNMLV->iItem, 0, LVIS_SELECTED);
                    ListView_SetCheckState( pNMHDR->hwndFrom, pNMLV->iItem, !ListView_GetCheckState(pNMHDR->hwndFrom, pNMLV->iItem) );
                }
            }
            return (INT_PTR)TRUE;
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    if (ValidateAndStoreSettings(hDlg))
                    {
                        EndDialog(hDlg, IDOK);
                    }
                    break;
                }
                case IDCANCEL:
                {
                    EndDialog(hDlg, IDCANCEL);
                    break;
                }
                case IDC_BUTTON_LOG_VERBOSITY:
                {
                    if (!logVerbositySelectorOpen)
                    {
                        RECT rect;
                        rect.left = 7;
                        MapDialogRect(hDlg, &rect);
                        hndCtrl = GetDlgItem( hDlg, IDC_LIST_LOG_VERBOSITY );
                        DWORD lvDims = ListView_ApproximateViewRect( hndCtrl, -1, -1, numLogVerbosityFlags );
                        SetWindowPos( hndCtrl, 0, 0, 0, LOWORD(lvDims)+rect.left, HIWORD(lvDims), SWP_NOZORDER | SWP_NOMOVE );
                        logVerbositySelectorOpen = true;
                    }
                    else
                    {
                        hndCtrl = GetDlgItem( hDlg, IDC_LIST_LOG_VERBOSITY );
                        SetWindowPos( hndCtrl, 0, 0, 0, LOWORD(ListView_ApproximateViewRect( hndCtrl, -1, -1, numLogVerbosityFlags )), 0, SWP_NOZORDER | SWP_NOMOVE);
                        logVerbositySelectorOpen = false;
                    }
                    break;
                }
                case IDC_EDIT_NOISE_REJECT_TIMEBASE:
                {
                    if (EN_CHANGE == HIWORD(wParam))
                    {
                        uint32_t newTimebase;
                        double newFrequency;
                        wchar_t newTimebaseText[16];
                        wchar_t sampleRateDisplayString[128];
                        Edit_GetText((HWND)lParam, newTimebaseText, 16);
                        if (wcslen(newTimebaseText))
                        {
                            newTimebase = _wtol(newTimebaseText);
                            if (pCurrentScope->GetFrequencyFromTimebase(newTimebase, newFrequency))
                            {
                                swprintf(sampleRateDisplayString, 128, L"Noise reject sample rate: %s", PrintSampleRate(newFrequency).c_str());
                            }
                            else
                            {
                                swprintf(sampleRateDisplayString, 128, L"Noise reject sample rate: INVALID");
                            }
                        }
                        else
                        {
                            swprintf(sampleRateDisplayString, 128, L"Noise reject sample rate: INVALID");
                        }
                        hndCtrl = GetDlgItem(hDlg, IDC_STATIC_NOISE_REJECT_SAMPLING_RATE);
                        Static_SetText(hndCtrl, sampleRateDisplayString);
                    }
                    break;
                }
                case IDC_BUTTON_AXIS_SETTINGS:
                {
                    PlotScaleSettings_T plotSettings;
                    DWORD dwDlgResp;

                    plotSettings.freqAxisScale = pSettings->GetFreqScale();
                    plotSettings.gainAxisScale = pSettings->GetGainScale();
                    plotSettings.phaseAxisScale = pSettings->GetPhaseScale();
                    plotSettings.freqAxisIntervals = pSettings->GetFreqIntervals();
                    plotSettings.gainAxisIntervals = pSettings->GetGainIntervals();
                    plotSettings.phaseAxisIntervals = pSettings->GetPhaseIntervals();
                    plotSettings.gainMasterIntervals = pSettings->GetGainMasterIntervals();
                    plotSettings.phaseMasterIntervals = pSettings->GetPhaseMasterIntervals();

                    dwDlgResp = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_AXES_DIALOG), hDlg, PlotAxesDialogHandler, (LPARAM)&plotSettings);

                    if (IDOK == dwDlgResp)
                    {
                        pSettings->SetFreqScale(plotSettings.freqAxisScale);
                        pSettings->SetGainScale(plotSettings.gainAxisScale);
                        pSettings->SetPhaseScale(plotSettings.phaseAxisScale);
                        pSettings->SetFreqIntervals(plotSettings.freqAxisIntervals);
                        pSettings->SetGainIntervals(plotSettings.gainAxisIntervals);
                        pSettings->SetPhaseIntervals(plotSettings.phaseAxisIntervals);
                        pSettings->SetGainMasterIntervals(plotSettings.gainMasterIntervals);
                        pSettings->SetPhaseMasterIntervals(plotSettings.phaseMasterIntervals);
                    }

                    break;
                }
                default:
                    break;
            }
            return (INT_PTR)TRUE;
            break;
        }
        default:
            return (INT_PTR)FALSE;
    }
}
