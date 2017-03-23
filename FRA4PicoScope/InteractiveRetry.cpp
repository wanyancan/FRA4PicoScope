//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2014-2017 by Aaron Hexamer
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
// Module InteractiveRetry.cpp: Contains functions handling the interactive retry dialog, which is
//                              used to control what happens when a step reaches a retry limit for
//                              auto-range or adaptive stimulus.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

// Suppress an unnecessary C4996 warning about use of checked
// iterators invoked from Boost
#if !defined(_SCL_SECURE_NO_WARNINGS)
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <Windows.h>
#include <Windowsx.h>
#include <boost/algorithm/string.hpp>
#include "FRA4PicoScopeInterfaceTypes.h"
#include "PicoScopeFraApp.h"
#include "Resource.h"

static bool bAdjustSetupOpen = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: AdjustDialogHeight
//
// Purpose: Adjusts the dialog height to show or hide the retry adjustment controls based on the
//          toggled state
//
// Parameters: [in] hDlg - Handle to the Interactive Retry dialog
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void AdjustDialogHeight( HWND hDlg )
{
    RECT dlgExistingRect;
    RECT dlgNewRect = {0, 0, 0, 0}; // Use bottom for window height, use top for button movement
    HWND hndBtnCtrl;
    
    if (!bAdjustSetupOpen)
    {
        dlgNewRect.bottom = 204;
        dlgNewRect.top = 116;
    }
    else
    {
        dlgNewRect.bottom = 320;
        dlgNewRect.top = 116;
    }
    MapDialogRect( hDlg, &dlgNewRect );

    if (!bAdjustSetupOpen)
    {
        dlgNewRect.top = -dlgNewRect.top;
    }

    // Resize Window
    GetWindowRect( hDlg, &dlgExistingRect );
    int width = dlgExistingRect.right - dlgExistingRect.left;
    SetWindowPos(hDlg, 0, 0, 0, width, dlgNewRect.bottom, SWP_NOZORDER | SWP_NOMOVE);

    // Move buttons
    hndBtnCtrl = GetDlgItem( hDlg, IDRETRY );
    GetWindowRect( hndBtnCtrl, &dlgExistingRect );
    MapWindowPoints( HWND_DESKTOP, GetParent(hndBtnCtrl), (LPPOINT)&dlgExistingRect, 2 );
    SetWindowPos( hndBtnCtrl, 0, dlgExistingRect.left, dlgExistingRect.top + dlgNewRect.top, 0, 0, SWP_NOSIZE );

    hndBtnCtrl = GetDlgItem( hDlg, IDCONTINUE );
    GetWindowRect( hndBtnCtrl, &dlgExistingRect );
    MapWindowPoints( HWND_DESKTOP, GetParent(hndBtnCtrl), (LPPOINT)&dlgExistingRect, 2 );
    SetWindowPos( hndBtnCtrl, 0, dlgExistingRect.left, dlgExistingRect.top + dlgNewRect.top, 0, 0, SWP_NOSIZE );

    hndBtnCtrl = GetDlgItem( hDlg, IDABORT );
    GetWindowRect( hndBtnCtrl, &dlgExistingRect );
    MapWindowPoints( HWND_DESKTOP, GetParent(hndBtnCtrl), (LPPOINT)&dlgExistingRect, 2 );
    SetWindowPos( hndBtnCtrl, 0, dlgExistingRect.left, dlgExistingRect.top + dlgNewRect.top, 0, 0, SWP_NOSIZE );

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GetAutorangeStatusString
//
// Purpose: Returns a string explaining the auto-range status
//
// Parameters: [in] status - the auto-range status
//             [out] return - the explanatory string
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

static const wchar_t autorangeStatusStrings[NUM_AUTORANGE_STATUS_VALUES+1][128] =
{
    L"OK",
    L"Highest range limit reached",
    L"Lowest range limit reached",
    L"Over-range",
    L"Amplitude too high, adjusting",
    L"Amplitude too low, adjusting",
    L"Unknown"
};

const wchar_t* GetAutorangeStatusString(AUTORANGE_STATUS_T status)
{

    if (status >= 0 && status < NUM_AUTORANGE_STATUS_VALUES)
    {
        return autorangeStatusStrings[status];
    }
    else
    {
        return autorangeStatusStrings[NUM_AUTORANGE_STATUS_VALUES];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: PrintVolts
//
// Purpose: Prints a voltage value to six digits precision.
//
// Parameters: [in] value - the voltage to print
//             [out] valueStr - the formatted output string
//
// Notes: None
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void PrintVolts(double value, std::wstring& valueStr)
{
    std::wstringstream valueSS;
    // Output using fixed precision
    valueSS.precision(6);
    valueSS << fixed << value;
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

    if (0 == valueStr.compare(L"0"))
    {
        valueStr = L"less than 1 uV";
    }
    else
    {
        valueStr += L" V";
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: InteractiveRetryHandler
//
// Purpose: Dialog procedure for the Settings Dialog.  Handles initialization and user actions.
//
// Parameters: See Windows API documentation
//
// Notes: N/A
//
///////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK InteractiveRetryHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            wchar_t statusText[128];
            std::wstring valueStr;
            FRA_STATUS_MESSAGE_T* pStatus = (FRA_STATUS_MESSAGE_T*)lParam;
            HWND hndTxtCtrl;

            if (!bAdjustSetupOpen) // Because the dialog template starts as open
            {
                AdjustDialogHeight(hDlg);
            }

            // For now, auto-ranging is always on, so display its parameters
            hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_AUTORANGE_N_OF_M );
            std::swprintf( statusText, 128, L"%hhu of %hhu attempts", pStatus->statusData.retryLimit.autorangeLimit.triesAttempted, pStatus->statusData.retryLimit.autorangeLimit.allowedTries );
            Static_SetText( hndTxtCtrl, statusText );

            hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_INPUT_CHANNEL_RANGE );
            std::swprintf( statusText, 128, L"Input channel range: %s", pStatus->statusData.retryLimit.autorangeLimit.pRangeInfo[pStatus->statusData.retryLimit.autorangeLimit.inputRange].name );
            Static_SetText( hndTxtCtrl, statusText );

            hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_OUTPUT_CHANNEL_RANGE );
            std::swprintf( statusText, 128, L"Output channel range: %s", pStatus->statusData.retryLimit.autorangeLimit.pRangeInfo[pStatus->statusData.retryLimit.autorangeLimit.outputRange].name );
            Static_SetText( hndTxtCtrl, statusText );

            hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_INPUT_CHANNEL_STATUS );
            std::swprintf( statusText, 128, L"Input channel status: %s", GetAutorangeStatusString( pStatus->statusData.retryLimit.autorangeLimit.inputChannelStatus ) );
            Static_SetText( hndTxtCtrl, statusText );

            hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_OUTPUT_CHANNEL_STATUS );
            std::swprintf( statusText, 128, L"Output channel status: %s", GetAutorangeStatusString( pStatus->statusData.retryLimit.autorangeLimit.outputChannelStatus ) );
            Static_SetText( hndTxtCtrl, statusText );

            if (pSettings->GetAdaptiveStimulusMode())
            {
                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_ADAPTIVE_STIMULUS_N_OF_M );
                std::swprintf( statusText, 128, L"%hhu of %hhu attempts", pStatus->statusData.retryLimit.adaptiveStimulusLimit.triesAttempted, pStatus->statusData.retryLimit.adaptiveStimulusLimit.allowedTries );
                Static_SetText( hndTxtCtrl, statusText );

                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_STIMULUS_VPP );
                PrintVolts( pStatus->statusData.retryLimit.adaptiveStimulusLimit.stimulusVpp, valueStr );
                std::swprintf( statusText, 128, L"Stimulus Vpp: %s", valueStr.c_str() );
                Static_SetText( hndTxtCtrl, statusText );

                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_INPUT_RESPONSE_AMPLITUDE );
                PrintVolts( pStatus->statusData.retryLimit.adaptiveStimulusLimit.inputResponseAmplitudeV, valueStr );
                std::swprintf( statusText, 128, L"Input response amplitude: %s", valueStr.c_str() );
                Static_SetText( hndTxtCtrl, statusText );

                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_OUTPUT_RESPONSE_AMPLITUDE );
                PrintVolts( pStatus->statusData.retryLimit.adaptiveStimulusLimit.outputResponseAmplitudeV, valueStr );
                std::swprintf( statusText, 128, L"Output response amplitude: %s", valueStr.c_str() );
                Static_SetText( hndTxtCtrl, statusText );
            }
            else
            {
                // Disable the controls
                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_AS_GRP );
                EnableWindow( hndTxtCtrl, FALSE );
                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_ADAPTIVE_STIMULUS_N_OF_M );
                EnableWindow( hndTxtCtrl, FALSE );
                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_STIMULUS_VPP );
                EnableWindow( hndTxtCtrl, FALSE );
                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_INPUT_RESPONSE_AMPLITUDE );
                EnableWindow( hndTxtCtrl, FALSE );
                hndTxtCtrl = GetDlgItem( hDlg, IDC_RL_OUTPUT_RESPONSE_AMPLITUDE );
                EnableWindow( hndTxtCtrl, FALSE );
            }
        }
        return (INT_PTR)TRUE;
        break;
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDRETRY || LOWORD(wParam) == IDCONTINUE || LOWORD(wParam) == IDABORT)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }

            if (LOWORD(wParam) == IDC_RL_ADJUST_SETUP_BUTTON)
            {
                bAdjustSetupOpen = !bAdjustSetupOpen;
                AdjustDialogHeight(hDlg);
                return (INT_PTR)TRUE;
            }

            return (INT_PTR)FALSE;
        }
        break;
        default:
            return (INT_PTR)FALSE;
            break;
    }
}