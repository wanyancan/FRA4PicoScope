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
#include <string>
#include <sstream>
#include "utility.h"
#include "SettingsDialog.h"
#include "PicoScopeFraApp.h"
#include "Resource.h"


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
    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND hndCtrl;

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

            return (INT_PTR)TRUE;
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
            }
            return (INT_PTR)TRUE;
            break;
        }
        default:
            return (INT_PTR)FALSE;
    }
}