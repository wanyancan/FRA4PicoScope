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
#include <Windowsx.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include "utility.h"
#include "SettingsDialog.h"
#include "PicoScopeFraApp.h"
#include "PicoScopeInterface.h"
#include "Resource.h"

const uint8_t numLogVerbosityFlags = 6;

const wchar_t logVerbosityString[numLogVerbosityFlags][128] =
{
    L"FRA Progress",
    L"Step Trial Progress",
    L"Autorange Diagnostics",
    L"Adaptive Stimulus Diagnostics",
    L"Sample Processing Diagnostics",
    L"DFT Diagnostics"
};

bool logVerbositySelectorOpen = false;
PicoScope* pCurrentScope = NULL;

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
            Edit_SetText( hndCtrl, pSettings->GetExtraSettlingTimeAsString().c_str() );

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
                int minRange, maxRange;
                const RANGE_INFO_T* rangeInfo = pCurrentScope->GetRangeCaps();
                minRange = pCurrentScope->GetMinRange(PS_AC);
                maxRange = pCurrentScope->GetMaxRange(PS_AC);

                hndCtrl = GetDlgItem( hDlg, IDC_COMBO_INPUT_START_RANGE );
                ComboBox_AddString( hndCtrl, L"Stimulus" );
                ComboBox_AddItemData( hndCtrl, -1 );
                for (int idx = minRange; idx <= maxRange; idx++)
                {
                    ComboBox_AddString( hndCtrl, rangeInfo[idx].name );
                    ComboBox_AddItemData( hndCtrl, idx );
                }
                ComboBox_SetCurSel( hndCtrl, ComboBox_FindItemData( hndCtrl, -1, pSettings->GetInputStartingRange() ) );

                hndCtrl = GetDlgItem( hDlg, IDC_COMBO_OUTPUT_START_RANGE );
                ComboBox_AddString( hndCtrl, L"Stimulus" );
                ComboBox_AddItemData( hndCtrl, -1 );
                for (int idx = minRange; idx <= maxRange; idx++)
                {
                    ComboBox_AddString( hndCtrl, rangeInfo[idx].name );
                    ComboBox_AddItemData( hndCtrl, idx );
                }
                ComboBox_SetCurSel( hndCtrl, ComboBox_FindItemData( hndCtrl, -1, pSettings->GetOutputStartingRange() ) );
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
                hndCtrl = GetDlgItem( hDlg, IDC_EDIT_NOISE_REJECT_TIMEBASE );
                Edit_SetText( hndCtrl, pSettings->GetNoiseRejectModeTimebaseAsString().c_str() );
            }
            else
            {
                hndCtrl = GetDlgItem( hDlg, IDC_EDIT_NOISE_REJECT_TIMEBASE );
                EnableWindow( hndCtrl, FALSE );
                hndCtrl = GetDlgItem( hDlg, IDC_STATIC_NOISE_REJECT_TIMEBASE );
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
            // Don't let items be selected in the log verbosity list
            NMHDR* pNMHDR = ((NMHDR*)lParam);
            if (IDC_LIST_LOG_VERBOSITY == pNMHDR->idFrom &&
                LVN_ITEMCHANGED == pNMHDR->code)
            {
                ListView_SetItemState(pNMHDR->hwndFrom, -1, 0, LVIS_SELECTED);
            }
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    EndDialog(hDlg, IDOK);
                }
                case IDCANCEL:
                {
                    EndDialog(hDlg, IDCANCEL);
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
                        break;
                    }
                    else
                    {
                        hndCtrl = GetDlgItem( hDlg, IDC_LIST_LOG_VERBOSITY );
                        SetWindowPos( hndCtrl, 0, 0, 0, LOWORD(ListView_ApproximateViewRect( hndCtrl, -1, -1, numLogVerbosityFlags )), 0, SWP_NOZORDER | SWP_NOMOVE);
                        logVerbositySelectorOpen = false;
                        break;
                    }
                    default:
                        break;
                }
            }
            return (INT_PTR)TRUE;
            break;
        }
        default:
            return (INT_PTR)FALSE;
    }
}