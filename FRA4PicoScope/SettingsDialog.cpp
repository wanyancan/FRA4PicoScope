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