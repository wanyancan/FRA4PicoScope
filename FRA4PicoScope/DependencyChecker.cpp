//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frequency Response Analyzer for PicoScope
//
// Copyright (c) 2016 by Aaron Hexamer
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
// Module DependencyChecker.cpp: Contains functions for checking the version of DLLs we depend on,
//                               and to warn the user when the correct dependencies weren't found.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <sstream>

// Temporarily redefining the target version so we can get access to Task Dialog interface
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#include <CommCtrl.h>
#undef _WIN32_WINNT

typedef HRESULT (WINAPI* TDIP)(__in const TASKDIALOGCONFIG *pTaskConfig, __out_opt int *pnButton, __out_opt int *pnRadioButton, __out_opt BOOL *pfVerificationFlagChecked);

typedef union
{
    WORD ver16[4];
    uint64_t ver64;
} ver_T;

typedef struct
{
    wstring name;
    ver_T ver;
} DependencyRecord_T;

// Note little endian byte order (used to enable simple integer comparison)
const DependencyRecord_T dependencies[] =
{
    { L"ps2000.dll", {38, 4, 1, 2} },
    { L"ps2000a.dll", {46, 4, 1, 1} },
    { L"ps3000.dll", { 28, 5, 7, 3 } },
    { L"ps3000a.dll", { 56, 4, 1, 1 } },
    { L"ps4000.dll", { 41, 4, 2, 1 } },
    { L"ps4000a.dll", { 48, 4, 0, 1 } },
    { L"ps5000.dll", { 32, 4, 5, 1 } },
    { L"ps5000a.dll", { 39, 4, 1, 1 } },
    { L"ps6000.dll", { 43, 4, 4, 1 } }
};

const wstring PreferredDependencyName = L"PicoScope SDK Version 10.6.10.22";

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GetDllVersion
//
// Purpose: Get the version from a DLL file
//
// Parameters: [in] dllPath: Path to the DLL file
//             [out] ver: the version of the file
//
// Notes: returns 0 if the version can't be retrieved
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void GetDllVersion(wstring dllPath, uint64_t& ver)
{
    DWORD verHandle = NULL;
    UINT size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD verSize = GetFileVersionInfoSize(dllPath.c_str(), &verHandle);

    ver = 0;

    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];
        if (GetFileVersionInfo(dllPath.c_str(), verHandle, verSize, verData))
        {
            if (VerQueryValue(verData, L"\\", (VOID FAR* FAR*)&lpBuffer, &size))
            {
                if (size)
                {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                    if (verInfo->dwSignature == 0xfeef04bd)
                    {
                        ver = (uint64_t)(verInfo->dwFileVersionMS) << 32 | verInfo->dwFileVersionLS;
                    }
                }
            }
        }
        delete[] verData;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: CheckDependencies
//
// Purpose: Check all the dependencies encoded in the dependency records.
//
// Parameters: N/A
//
// Notes: dependency checks pass if the versions found are greater than or equal to required
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CheckDependencies( void )
{
    bool depStatus = true;
    bool retVal = true;
    ver_T ver;
    HMODULE hModule;
    TCHAR dllPath[_MAX_PATH];
    wstring problems, problemDetails;

    for each (DependencyRecord_T dep in dependencies)
    {
        hModule = GetModuleHandle(dep.name.c_str());
        GetModuleFileName(hModule, dllPath, _MAX_PATH);

        GetDllVersion(dllPath, ver.ver64);

        if (ver.ver64 < dep.ver.ver64)
        {
            wstringstream vssActual, vssExpected;
            vssExpected << dep.ver.ver16[3] << L"." << dep.ver.ver16[2] << L"." << dep.ver.ver16[1] << L"." << dep.ver.ver16[0];
            vssActual << ver.ver16[3] << L"." << ver.ver16[2] << L"." << ver.ver16[1] << L"." << ver.ver16[0];
            problemDetails += wstring(dllPath) + L" version " + vssActual.str() + L" is less than " + vssExpected.str() + L"\n";
            depStatus = false;
        }
    }
    if (!depStatus)
    {
        problems = L"Loaded DLLs do not meet minimum compatible version.  Please install preferred dependency: " + PreferredDependencyName;

        // Try to use a TaskDialog, which should be possible with Vista or later
        TDIP TaskDialogIndirectProc = NULL;
        HMODULE hMod = GetModuleHandle(L"comctl32.dll");
        TaskDialogIndirectProc = (TDIP)GetProcAddress(hMod, "TaskDialogIndirect");
        if (TaskDialogIndirectProc)
        {
            int dlgResult;
            wstring message = problems + L"\n\nContinue?";
            TASKDIALOGCONFIG config = { 0 };
            config.cbSize = sizeof(config);
            config.hInstance = GetModuleHandle(NULL);
            config.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
            config.pszMainIcon = TD_WARNING_ICON;
            config.pszMainInstruction = message.c_str();
            config.pszContent = NULL;
            config.pszExpandedInformation = problemDetails.c_str();
            config.pszExpandedControlText = L"Hide Details";
            config.pszCollapsedControlText = L"Show Details";
            config.pButtons = NULL;
            config.cButtons = 0;
            TaskDialogIndirectProc(&config, &dlgResult, NULL, NULL);
            if (IDNO == dlgResult)
            {
                retVal = false;
            }
        }
        else // Fallback to MessageBox
        {
            wstring message = problems + L"\n\nContinue?";
            if (IDNO == MessageBox(NULL, message.c_str(), L"Dependency Error", MB_YESNO | MB_ICONEXCLAMATION))
            {
                retVal = false;
            }
        }
    }
    return retVal;
}