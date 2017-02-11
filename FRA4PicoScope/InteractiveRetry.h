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
// Module InteractiveRetry.h: Contains functions handling the interactive retry dialog, which is
//                            used to control what happens when a step reaches a retry limit for
//                            auto-range or adaptive stimulus.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Windows.h>

INT_PTR CALLBACK InteractiveRetryHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
