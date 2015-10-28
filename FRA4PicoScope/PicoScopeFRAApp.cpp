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
// Module PicoScopeFraApp.cpp: Main module for the application.
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <csignal>
#include <windowsx.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <delayimp.h>
#include <iomanip>
#include <fstream>

#include "PicoScopeFRA.h"
#include "utility.h"
#include "StatusLog.h"
#include "ApplicationSettings.h"
#include "ScopeSelector.h"
#include "PicoScopeFraApp.h"
#include "FRAPlotter.h"
#include "PlotAxesDialog.h"

//#define TEST_PLOTTING

#if defined(TEST_PLOTTING)
const static uint32_t testPlotN = 41;
double testPlotFreqs[];
double testPlotGains[];
double testPlotPhases[];
double testPlotUnwrappedPhases[];
#endif

char* appVersionString = "0.4b";
char* appNameString = "Frequency Response Analyzer for PicoScope";

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND hMainWnd = (HWND)NULL;

wstring dataDirectoryName;

TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

const int numAttens = 6;

// Application Objects
PicoScopeFRA* psFRA = NULL;
FRAPlotter* fraPlotter = NULL;
ScopeSelector* pScopeSelector = NULL;
ApplicationSettings* pSettings = NULL;

// Plot variables
bool plotAvailable = false; // true when a plot is available on the screen
bool initialPlot = false; // true when the plot displayed has not been manipulated by the user
HBITMAP hPlotBM;
HBITMAP hPlotUnavailableBM;
HBITMAP hPlotSeparatorBM;
int pxPlotXStart;
int pxPlotYStart;
int pxPlotWidth;
int pxPlotHeight;
int pxPlotSeparatorXStart;

HANDLE hExecuteFraThread;
HANDLE hExecuteFraEvent;

// Plot zoom variables
bool detectingZoom = false;
bool zooming = false;
POINT ptZoomBegin;
POINT ptZoomEnd;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
BOOL CALLBACK       WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void                Cleanup( HWND hWnd );
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ScopeSelectHandler(HWND, UINT, WPARAM, LPARAM);
BOOL                InitializeControls(HWND hDlg);
BOOL                LoadControlsData(HWND hDlg);
void                CopyLog(void);
void                ClearLog(void);

DWORD WINAPI        ExecuteFRA(LPVOID lpdwThreadParam);
bool                GeneratePlot(bool rescale);
void                RepaintPlot( void );
void                InitScope( void );
void                SelectNewScope( uint8_t index );
bool                FraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatus );
bool                ValidateSettings(void);
bool                StoreSettings(void);

void                SaveRawData( wstring dataFilePath );
void                SavePlotImageFile( wstring dataFilePath );
void                EnableAllMenus( void );
void                DisableAllMenus( void );

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SignalHandler
//
// Purpose: Handle signals SIGABRT, SIGTERM, and SIGINT
//
// Parameters: signal being handled
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void SignalHandler( int signum )
{
    Cleanup( hMainWnd );
    exit( signum );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: _tWinMain
//
// Purpose: Application Entry point
//
// Parameters: See Windows programming API information
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
    int status;
    HACCEL hAccelTable;

    WCHAR szProgramFilesFolder[MAX_PATH];
    HRESULT hr;

    // This code is to control where the PicoScope DLLs are loaded from.  We do this so
    // that we can reliably set a preference for the ones from the installed SDK.  To do
    // this, the PicoScope DLLs are delay loaded so that we can add the SDK lib folder in
    // the DLL search path before they get loaded.  According to some sources, other methods
    // such as manifests won't work to target DLLs by specific path.
#if defined(_M_X64)
    hr = SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szProgramFilesFolder);
#else
    hr = SHGetFolderPath(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, szProgramFilesFolder);
#endif
    if(SUCCEEDED(hr))
    {
        wstring wsDllPath = szProgramFilesFolder + wstring(L"\\Pico Technology\\SDK\\lib");
        BOOL bRes = SetDllDirectory( wsDllPath.c_str() );
        if (!bRes)
        {
            MessageBox( 0, L"Could not set application DLL folder.", L"Fatal Error", MB_OK );
            exit(-1);
        }
        else
        {
            // Load the DLLs so that we can do all error handling in one spot
            if (FAILED(__HrLoadAllImportsForDll("ps2000.dll")) || FAILED(__HrLoadAllImportsForDll("ps2000a.dll")) || FAILED(__HrLoadAllImportsForDll("PS3000.dll")) ||
                FAILED(__HrLoadAllImportsForDll("ps3000a.dll")) || FAILED(__HrLoadAllImportsForDll("ps4000.dll")) || FAILED(__HrLoadAllImportsForDll("ps4000a.dll")) ||
                FAILED(__HrLoadAllImportsForDll("ps5000.dll")) || FAILED(__HrLoadAllImportsForDll("ps5000a.dll")) || FAILED(__HrLoadAllImportsForDll("ps6000.dll")) )
            {
                MessageBox( 0, L"Could not load application DLLs.", L"Fatal Error", MB_OK );
                exit(-1);
            }
        }
    }
    else
    {
        MessageBox( 0, L"Could not set application DLL folder.", L"Fatal Error", MB_OK );
        exit(-1);
    }

    // Install signal handler for abort, terminate, and interrupt
    signal( SIGABRT, SignalHandler );
    signal( SIGTERM, SignalHandler );
    signal( SIGINT, SignalHandler );

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_PICOSCOPEFRA, szWindowClass, MAX_LOADSTRING);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PICOSCOPEFRA));

    // Main message loop:
    while (status = GetMessage(&msg, NULL, 0, 0))
    {
        if (status == -1)
        {
            return -1;
        }
        if (!IsDialogMessage(hMainWnd, &msg))
        {
            TranslateMessage ( &msg );
            DispatchMessage ( &msg );
        }
    }

    return (int) msg.wParam;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SelectNewScope
//
// Purpose: Set the scope selected for use in the FRA
//
// Parameters: [in] scope: Details on the scope to swith to
//             [in] mruScope: Whether this is the most recently used scope (from the application 
//                            configuration file)
//
// Notes: Setting mruScope to true makes failure not be final (i.e., we won't report inability
//        to open the scope to the user)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void SelectNewScope( AvailableScopeDescription_T scope, bool mruScope = false )
{
    PicoScope* pScope;

    // If there is already a scope selected, close it.
    // TBD: Could implement a previous selected scope concept in the scope selector, so that
    // we can try to open the new scope first and test for success before closing the old one.
    // In that case, we'd also need to build in a way to revert the currently selected scope to
    // the previous scope
    if (pScopeSelector->GetSelectedScope())
    {
        StoreSettings();
        pSettings->WriteScopeSettings();
        pScopeSelector->GetSelectedScope()->Close();
        delete pScopeSelector->GetSelectedScope();
    }

    pScope = pScopeSelector -> OpenScope(scope);
    if (!pScope)
    {
        if (!mruScope) // If it's the most recently used scope we're tryign to open, the failure is not final.
        {
            wstringstream statusMessage;
            statusMessage << L"Error: Unable to initialize device with serial number " << scope.wSerialNumber.c_str();
            LogMessage( statusMessage.str() );

            pSettings->SetMostRecentScope( PS_NO_FAMILY, string("None") );
            pSettings->SetNoScopeSettings();
            LoadControlsData(hMainWnd);
        }
    }
    else
    {
        psFRA->SetInstrument(pScope);

        wstringstream statusMessage;
        wstring scopeModel, scopeSN;
        pScope->GetModel(scopeModel);
        pScope->GetSerialNumber(scopeSN);
        statusMessage << L"Status: " << scopeModel.c_str() << L" S/N: " << scopeSN.c_str() << L" successfully initialized.";
        LogMessage( statusMessage.str() );

        if (false == pSettings -> ReadScopeSettings( pScope ))
        {
            MessageBox( 0, L"Could not read or initialize device settings file.", L"Fatal Error", MB_OK );
            exit(-1);
        }

        if (!mruScope) // Don't unnecessarily dirty the settings file
        {
            pSettings->SetMostRecentScope(scope.driverFamily, scope.serialNumber);
        }

        LoadControlsData(hMainWnd);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: InitScope
//
// Purpose: Handle scope initialization when the application is opened.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void InitScope( void )
{
    wstring statusMessage;
    ScopeDriverFamily_T mostRecentScopeFamily;
    string mostRecentScopeSN;

    pSettings->GetMostRecentScope( mostRecentScopeFamily, mostRecentScopeSN );

    if (0 != mostRecentScopeSN.compare("None")) // If there is a most recently used Scope
    {
        AvailableScopeDescription_T mruScope;
        mruScope.driverFamily = mostRecentScopeFamily;
        mruScope.serialNumber = mostRecentScopeSN;
        SelectNewScope( mruScope, true );
    }

    // Check to see if the above attempt to open the most recently used scope was successful.
    // If not, look for other available scopes.
    if (!(pScopeSelector->GetSelectedScope()))
    {
        vector<AvailableScopeDescription_T> scopes;

        pScopeSelector -> GetAvailableScopes( scopes );

        // It would be nice to add logic here to check whether any of the scopes found are compatible. However,
        // the PicoScope API doesn't support getting the model number during enumeration (without opening the device)
        // and thus, without other API enhancements, we can't tell whether a scope is compatible.  In leiu of that,
        // until there's a better method, we'll detect compatibility upon opening the scope, then check it before
        // executing the FRA
        if (scopes.size() == 1)
        {
            SelectNewScope(scopes[0]);
        }
        else if (scopes.size() > 1)
        {
            DWORD dwDlgResp;
            dwDlgResp = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SCOPESELECTORBOX), NULL, ScopeSelectHandler, (LPARAM)&scopes);
            if (LOWORD(dwDlgResp) == IDOK)
            {
                SelectNewScope(scopes[HIWORD(dwDlgResp)]);
            }
            else
            {
                statusMessage = L"Error: No PicoScope device selected.";
                LogMessage( statusMessage );
                pSettings -> SetNoScopeSettings();
                LoadControlsData(hMainWnd);
            }
        }
        else
        {
            statusMessage = L"Error: No compatible PicoScope device found.";
            LogMessage( statusMessage );
            pSettings -> SetNoScopeSettings();
            LoadControlsData(hMainWnd);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: InitInstance
//
// Purpose: Initialize application parameters for this instance of the application, and create the 
//          main program window.
//
// Parameters: Refer to common windows programming structure
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    WCHAR szDocumentsFolder[MAX_PATH];

    hInst = hInstance; // Store instance handle in our global variable

    // Create execution thread and event
    hExecuteFraEvent = CreateEventW( NULL, true, false, L"ExecuteFRA" );

    if ((HANDLE)NULL == hExecuteFraEvent)
    {
        MessageBox( 0, L"Could not initialize application event \"ExecuteFRA\".", L"Fatal Error", MB_OK );
        delete pSettings;
        exit(-1);
    }
    else if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        MessageBox( 0, L"Cannot run multiple instances of FRA4PicoScope.", L"Fatal Error", MB_OK );
        delete pSettings;
        exit(-1);
    }

    if( !ResetEvent( hExecuteFraEvent ) )
    {
        MessageBox( 0, L"Could not reset application event \"ExecuteFRA\".", L"Fatal Error", MB_OK );
        delete pSettings;
        exit(-1);
    }

    hExecuteFraThread = CreateThread( NULL, 0, ExecuteFRA, NULL, 0, NULL );

    if ((HANDLE)NULL == hExecuteFraThread)
    {
        MessageBox( 0, L"Could not initialize application FRA execution thread.", L"Fatal Error", MB_OK );
        delete pSettings;
        exit(-1);
    }

    // Get the path of the folder we'll use to store data (raw FRA data, plot image files, etc)
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szDocumentsFolder)))
    {
        dataDirectoryName = szDocumentsFolder;
        dataDirectoryName.append(L"\\FRA4PicoScope\\");
    }
    else
    {
        MessageBox( 0, L"Could not initialize application data folder.", L"Fatal Error", MB_OK );
        exit(-1);
    }

    pScopeSelector = new ScopeSelector();

    // Create the main window
    hMainWnd = CreateDialog( hInst, MAKEINTRESOURCE(IDD_MAIN), 0, WndProc );

    if (!hMainWnd)
    {
        return FALSE;
    }

#if defined(TEST_LOG)
    FRA_STATUS_MESSAGE_T fraMsg;
    fraMsg.status = FRA_STATUS_MESSAGE;
    fraMsg.statusText = L"Test Message 1";
    FraStatusCallback( fraMsg );
    fraMsg.statusText = L"Test Message 2";
    FraStatusCallback( fraMsg );
    fraMsg.statusText = L"Test Message 3";
    FraStatusCallback( fraMsg );
#if defined( TEST_LOG_CAPACITY )
    for (int i = 0; i < 1000000; i++)
    {
        fraMsg.statusText = L"123456"; // including CR/LF, this should be 8 chars
        FraStatusCallback( fraMsg );
    }
#endif
#endif

    // Center the window, taking into consideration the actual window size
    RECT mainWindowRect;
    GetWindowRect( hMainWnd, &mainWindowRect );
    int xPos = (GetSystemMetrics(SM_CXSCREEN) - (mainWindowRect.right - mainWindowRect.left))/2;
    int yPos = (GetSystemMetrics(SM_CYSCREEN) - (mainWindowRect.bottom - mainWindowRect.top))/2;

    SetWindowPos( hMainWnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: DrawZoomRectangle
//
// Purpose: Erases the previously drawn zoom rectangle or draws a new one
//
// Parameters: [in] hWnd: Handle to the window on which the rectangle is drawn
//             [in] ptZoomBegin: coordinates for start of zoom rectangle
//             [in] ptZoomEnd: coordinate for end of zoom rectangle
//
// Notes: Used by the code that handles user mouse drag operations for zooming on the plot.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void DrawZoomRectangle (HWND hWnd, POINT ptZoomBegin, POINT ptZoomEnd)
{
     HDC hdc;

     hdc = GetDC( hWnd );

     SetROP2( hdc, R2_NOT );
     SelectObject( hdc, GetStockObject(NULL_BRUSH) );
     Rectangle( hdc, ptZoomBegin.x, ptZoomBegin.y, ptZoomEnd.x, ptZoomEnd.y );

     ReleaseDC( hWnd, hdc );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: WndProc
//
// Purpose: Message handler for the main window
//
// Parameters: See Windows programming API information
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    HMENU hMenu;
    PAINTSTRUCT ps;
    HDC hdc;
    HDC hdcMem;
    HBITMAP hbmOld;
    vector<AvailableScopeDescription_T> scopes;

    switch (message)
    {
        case WM_INITDIALOG:
            WCHAR szAppDataFolder[MAX_PATH];

            hMainWnd = hWnd; // Doing this here so that FraStatusCallback works when construct the objects below

            hMenu = LoadMenu( hInst, MAKEINTRESOURCE(IDC_PICOSCOPEFRA));
            SetMenu( hWnd, hMenu );

            InitializeControls(hWnd);

#if !defined(TEST_PLOTTING)
            try
            {
                psFRA = new PicoScopeFRA(FraStatusCallback);
            }
            catch (runtime_error e)
            {
                UNREFERENCED_PARAMETER(e);
                MessageBox( 0, L"Could not initialize FRA execution subsystem.", L"Fatal Error", MB_OK );
                exit(-1);
            }
#endif
            if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szAppDataFolder)))
            {
                pSettings = new ApplicationSettings( szAppDataFolder );
            }
            else
            {
                MessageBox( 0, L"Could not initialize application settings folder.", L"Fatal Error", MB_OK );
                exit(-1);
            }

            if (!(pSettings->ReadApplicationSettings()))
            {
                MessageBox( 0, L"Could not read or initialize application settings file.", L"Fatal Error", MB_OK );
                exit(-1);
            }
#if !defined(TEST_PLOTTING)
            psFRA->SetFraSettings( pSettings->GetSamplingMode(), pSettings->GetPurityLowerLimit(), pSettings->GetExtraSettlingTimeMs(),
                                   pSettings->GetAutorangeTriesPerStep(), pSettings->GetAutorangeTolerance(), pSettings->GetSmallSignalResolutionLimit(),
                                   pSettings->GetMaxAutorangeAmplitude(), pSettings->GetMinCyclesCaptured(), pSettings->GetSweepDescending(),
                                   pSettings->GetPhaseWrappingThreshold(), pSettings->GetTimeDomainPlotsEnabled(), dataDirectoryName );
            InitScope();
#else
            HWND hndCtrl;
            hndCtrl = GetDlgItem(hWnd, IDC_FRA_AUTO_AXES);
            Button_SetCheck( hndCtrl, true );
            hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_GAIN);
            Button_SetCheck( hndCtrl, true );
            hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_PHASE);
            Button_SetCheck( hndCtrl, true );
            hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_GM);
            Button_SetCheck( hndCtrl, true );
            hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_PM);
            Button_SetCheck( hndCtrl, true );
#endif

            fraPlotter = new FRAPlotter(pxPlotWidth, pxPlotHeight);
            if (!fraPlotter || !(fraPlotter->Initialize()))
            {
                MessageBox( 0, L"Could not initialize plotting subsystem.", L"Fatal Error", MB_OK );
                exit(-1);
            }

            hPlotBM = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_PLOT_UNAVAILABLE), IMAGE_BITMAP, pxPlotWidth, pxPlotHeight, 0);
            hPlotSeparatorBM = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_SEPARATOR), IMAGE_BITMAP, 2, pxPlotHeight, 0);

            return TRUE;
        case WM_COMMAND:
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    return TRUE;
                case IDM_CONNECT_SCOPE:
                    pScopeSelector->GetAvailableScopes(scopes);
                    if (scopes.size() == 0)
                    {
                        MessageBox( hWnd, L"No compatible PicoScope device found.", L"Error", MB_OK | MB_ICONEXCLAMATION );
                    }
                    else
                    {
                        DWORD dwDlgResp;
                        dwDlgResp = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SCOPESELECTORBOX), hWnd, ScopeSelectHandler, (LPARAM)&scopes);
                        if (LOWORD(dwDlgResp) == IDOK)
                        {
                            SelectNewScope(scopes[HIWORD(dwDlgResp)]);
                        }
                    }
                    return TRUE;
                case IDM_SAVE:
                {
                    time_t now;
                    struct tm  currentTime;
                    wchar_t dataFileName[128];
                    now = time(0);
                    localtime_s( &currentTime, &now );
                    wcsftime( dataFileName, sizeof(dataFileName)/sizeof(wchar_t), L"FRA Data %B %d, %Y %H-%M-%S.png", &currentTime );
                    SavePlotImageFile(dataDirectoryName+dataFileName);
                    return TRUE;
                }
                case IDM_SAVEAS:
                {
                    if (!fraPlotter->PlotDataAvailable())
                    {
                        MessageBox( 0, L"No plot is available to save.", L"Error", MB_OK );
                    }
                    else
                    {
                        OPENFILENAME ofn;
                        WCHAR szFileName[MAX_PATH] = L"";
                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.lpstrFilter = (LPCWSTR)L"PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0";
                        ofn.lpstrFile = (LPWSTR)szFileName;
                        ofn.lpstrInitialDir = dataDirectoryName.c_str();
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
                        ofn.lpstrDefExt = (LPCWSTR)L"png";

                        if (GetSaveFileName( &ofn ))
                        {
                            SavePlotImageFile(ofn.lpstrFile);
                        }
                    }
                    return TRUE;
                }
                case IDM_EXPORTDATA:
                {
                    time_t now;
                    struct tm  currentTime;
                    wchar_t dataFileName[128];
                    now = time(0);
                    localtime_s( &currentTime, &now );
                    wcsftime( dataFileName, sizeof(dataFileName)/sizeof(wchar_t), L"FRA Data %B %d, %Y %H-%M-%S.csv", &currentTime );
                    SaveRawData(dataDirectoryName+dataFileName);
                    return TRUE;
                }
                case IDM_EXPORTDATAAS:
                {
                    int numSteps;
                    double *freqsLogHz, *phasesDeg, *unwrappedPhasesDeg, *gainsDb;

                    psFRA->GetResults( &numSteps, &freqsLogHz, &gainsDb, &phasesDeg, &unwrappedPhasesDeg );

                    if (numSteps == 0)
                    {
                        MessageBox( 0, L"No data is available to export.", L"Error", MB_OK );
                    }
                    else
                    {
                        OPENFILENAME ofn;
                        WCHAR szFileName[MAX_PATH] = L"";
                        ZeroMemory(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.lpstrFilter = (LPCWSTR)L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
                        ofn.lpstrFile = (LPWSTR)szFileName;
                        ofn.lpstrInitialDir = dataDirectoryName.c_str();
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
                        ofn.lpstrDefExt = (LPCWSTR)L"csv";

                        if (GetSaveFileName( &ofn ))
                        {
                            SaveRawData(ofn.lpstrFile);
                        }
                    }
                    return TRUE;
                }
                case IDM_EXIT:
                    Cleanup(hWnd);
                    return TRUE;
                case IDC_CANCEL:
                    psFRA -> CancelFRA();
                    return TRUE;
                case IDC_GO:
                    if (WaitForSingleObject(hExecuteFraEvent, 0) == WAIT_TIMEOUT) // Is the event not already signalled?
                    {
                        if (BST_CHECKED == IsDlgButtonChecked( hMainWnd, IDC_AUTOCLEAR ))
                        {
                            ClearLog();
                        }
                        SetEvent( hExecuteFraEvent );
                    }
                    return TRUE;
                case IDM_CAL:
                    return TRUE;
                case IDC_COPY:
                    CopyLog();
                    return TRUE;
                case IDC_CLEAR:
                    ClearLog();
                    return TRUE;
                case IDC_FRA_AUTO_AXES:
                case IDC_FRA_SAVED_AXES:
                case IDC_FRA_DRAW_GAIN:
                case IDC_FRA_DRAW_PHASE:
                case IDC_FRA_DRAW_GM:
                case IDC_FRA_DRAW_PM:
                case IDC_FRA_UNWRAP_PHASE:
                    HWND hndCtrl;
                    StoreSettings();

                    hndCtrl = GetDlgItem(hWnd, IDC_FRA_UNWRAP_PHASE);
                    Button_Enable( hndCtrl, pSettings->GetPlotPhase() );

                    if (pSettings->GetPlotGain() && pSettings->GetPlotPhase() && !pSettings->GetPlotUnwrappedPhase())
                    {
                        hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_GM);
                        Button_Enable( hndCtrl, true );
                        hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_PM);
                        Button_Enable( hndCtrl, true );
                    }
                    else
                    {
                        hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_GM);
                        Button_SetCheck( hndCtrl, false );
                        Button_Enable( hndCtrl, false );
                        hndCtrl = GetDlgItem(hWnd, IDC_FRA_DRAW_PM);
                        Button_SetCheck( hndCtrl, false );
                        Button_Enable( hndCtrl, false );
                    }

                    // Store again since they may have changed
                    StoreSettings();

                    if (fraPlotter->PlotDataAvailable())
                    {
                        bool rescale = wmId == IDC_FRA_AUTO_AXES ||
                                       wmId == IDC_FRA_SAVED_AXES ||
                                       (pSettings->GetAutoAxes() && initialPlot);
                        (void)GeneratePlot(rescale);
                        RepaintPlot();
                    }
                    return TRUE;
                case IDCANCEL:
                    {
                        // Esc key was pressed.  Cancel zoom operation if it's happening.
                        if (zooming)
                        {
                            DrawZoomRectangle( hWnd, ptZoomBegin, ptZoomEnd );
                            ReleaseCapture();
                            SetCursor(LoadCursor(NULL, IDC_ARROW));
                            zooming = false;
                            detectingZoom = false;
                        }
                    }
                    return TRUE;
                default:
                    return FALSE;
            }
            return TRUE;
            break;
        case WM_LBUTTONDOWN:
            {
                // Check to see if it was a click on the plot area.
                ptZoomBegin.x = GET_X_LPARAM(lParam);
                ptZoomBegin.y = GET_Y_LPARAM(lParam);
                if (plotAvailable && ptZoomBegin.x >= pxPlotXStart && ptZoomBegin.x < pxPlotXStart+pxPlotWidth && ptZoomBegin.y >= pxPlotYStart && ptZoomBegin.y < pxPlotYStart+pxPlotHeight)
                {
                    ptZoomEnd.x = ptZoomBegin.x;
                    ptZoomEnd.y = ptZoomBegin.y;
          
                    SetCapture (hWnd) ;
                    SetCursor (LoadCursor (NULL, IDC_CROSS)) ;
          
                    detectingZoom = true;
                }
            }
            break;
        case WM_MOUSEMOVE:
            {
                POINT ptPossibleZoomEnd = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                // Clip it if it goes outside the plot area.
                if (ptPossibleZoomEnd.x < pxPlotXStart)
                {
                    ptPossibleZoomEnd.x = pxPlotXStart;
                }
                if (ptPossibleZoomEnd.x >= pxPlotXStart+pxPlotWidth)
                {
                    ptPossibleZoomEnd.x = pxPlotXStart+pxPlotWidth-1;
                }
                if (ptPossibleZoomEnd.y < pxPlotYStart)
                {
                    ptPossibleZoomEnd.y = pxPlotYStart;
                }
                if (ptPossibleZoomEnd.y >= pxPlotYStart+pxPlotHeight)
                {
                    ptPossibleZoomEnd.y = pxPlotYStart+pxPlotHeight-1;
                }

                if (detectingZoom)
                {
                    // Detect if the mouse moved enough to consider it a zoom operation
                    if (abs(ptPossibleZoomEnd.x-ptZoomBegin.x) > 5 && abs(ptPossibleZoomEnd.y-ptZoomBegin.y) > 5)
                    {
                        if (zooming)
                        {
                            // Erase the old rectangle
                            DrawZoomRectangle( hWnd, ptZoomBegin, ptZoomEnd );
                        }
                        else
                        {
                            zooming = true;
                        }
               
                        ptZoomEnd = ptPossibleZoomEnd;

                        // Draw the new rectangle
                        DrawZoomRectangle( hWnd, ptZoomBegin, ptZoomEnd );
                    }
                    else
                    { 
                        if (zooming)
                        {
                            // Erase the last rectangle as we go out of zoom mode
                            DrawZoomRectangle( hWnd, ptZoomBegin, ptZoomEnd );
                            zooming = false;
                        }
                    }
                }
            }
            return 0;
            break;
        case WM_LBUTTONUP:
            {
                if (detectingZoom)
                {
                    ReleaseCapture();
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                    detectingZoom = false;
                    zooming = false;

                    ptZoomEnd.x = GET_X_LPARAM(lParam);
                    ptZoomEnd.y = GET_Y_LPARAM(lParam);
                    // Clip it if it ends up outside the plot area.
                    if (ptZoomEnd.x < pxPlotXStart)
                    {
                        ptZoomEnd.x = pxPlotXStart;
                    }
                    if (ptZoomEnd.x >= pxPlotXStart+pxPlotWidth)
                    {
                        ptZoomEnd.x = pxPlotXStart+pxPlotWidth-1;
                    }
                    if (ptZoomEnd.y < pxPlotYStart)
                    {
                        ptZoomEnd.y = pxPlotYStart;
                    }
                    if (ptZoomEnd.y >= pxPlotYStart+pxPlotHeight)
                    {
                        ptZoomEnd.y = pxPlotYStart+pxPlotHeight-1;
                    }

                    if (abs(ptZoomEnd.x-ptZoomBegin.x) > 5 && abs(ptZoomEnd.y-ptZoomBegin.y) > 5)
                    {
                        // Consider it a zoom
                        if (fraPlotter->Zoom(make_tuple(pxPlotXStart, pxPlotYStart),
                                             make_tuple(pxPlotXStart+pxPlotWidth, pxPlotYStart+pxPlotHeight),
                                             make_tuple(ptZoomBegin.x, ptZoomBegin.y),
                                             make_tuple(ptZoomEnd.x, ptZoomEnd.y)))
                        {
                            RepaintPlot();
                            initialPlot = false;
                        }
                        else
                        {
                            // Erase the rectangle
                            DrawZoomRectangle( hWnd, ptZoomBegin, ptZoomEnd );
                        }
                    }
                    else // No zoom, just a single click
                    {
                        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                        // Check to see if it was a click on the plot area.
                        if (pt.x >= pxPlotXStart && pt.x < pxPlotXStart+pxPlotWidth && pt.y >= pxPlotYStart && pt.y < pxPlotYStart+pxPlotHeight)
                        {
                            PlotScaleSettings_T plotSettings;
                            DWORD dwDlgResp;
                            fraPlotter->GetPlotScaleSettings(plotSettings);
                            dwDlgResp = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_AXES_DIALOG), hWnd, PlotAxesDialogHandler, (LPARAM)&plotSettings);
                            if (LOWORD(dwDlgResp) == IDOK)
                            {
                                // Set the new settings and draw the plot
                                try
                                {
                                    fraPlotter->PlotFRA( NULL, NULL, NULL, 0,
                                                         plotSettings.freqAxisScale, plotSettings.gainAxisScale, plotSettings.phaseAxisScale,
                                                         plotSettings.freqAxisIntervals, plotSettings.gainAxisIntervals, plotSettings.phaseAxisIntervals,
                                                         plotSettings.gainMasterIntervals, plotSettings.phaseMasterIntervals );
                                }
                                catch (runtime_error e)
                                {
                                    wstringstream wss;
                                    wss << e.what();
                                    LogMessage( wss.str() );
                                    return 0;
                                }
                                RepaintPlot();
                                initialPlot = false;
                            }
                            return TRUE;
                        }
                        else
                        {
                            return FALSE;
                        }
                    }
                }
            }
            return 0; 
            break;
        case WM_RBUTTONUP:
            {
                ptZoomBegin.x = GET_X_LPARAM(lParam);
                ptZoomBegin.y = GET_Y_LPARAM(lParam);
                if (plotAvailable && ptZoomBegin.x >= pxPlotXStart && ptZoomBegin.x < pxPlotXStart+pxPlotWidth && ptZoomBegin.y >= pxPlotYStart && ptZoomBegin.y < pxPlotYStart+pxPlotHeight &&
                    fraPlotter->UndoOneZoom())
                {
                    RepaintPlot();
                }
            }
            return 0;
            break;
        case WM_RBUTTONDBLCLK:
            {
                ptZoomBegin.x = GET_X_LPARAM(lParam);
                ptZoomBegin.y = GET_Y_LPARAM(lParam);
                if (plotAvailable && ptZoomBegin.x >= pxPlotXStart && ptZoomBegin.x < pxPlotXStart+pxPlotWidth && ptZoomBegin.y >= pxPlotYStart && ptZoomBegin.y < pxPlotYStart+pxPlotHeight &&
                    fraPlotter->ZoomToOriginal())
                {
                    RepaintPlot();
                }
            }
            return 0;
            break;
        case WM_HSCROLL:
            return 0;
            break;
        case WM_PAINT:
            BITMAP bm;
            hdc = BeginPaint(hWnd, &ps);

            if (detectingZoom)
            {
                if (abs(ptZoomEnd.x-ptZoomBegin.x) > 5 || abs(ptZoomEnd.y-ptZoomBegin.y) > 5)
                {
                    SetROP2( hdc, R2_NOT ) ;
                    SelectObject( hdc, GetStockObject (NULL_BRUSH) );
                    Rectangle( hdc, ptZoomBegin.x, ptZoomBegin.y, ptZoomEnd.x, ptZoomEnd.y ) ;
                }
            }

            hdcMem = CreateCompatibleDC(hdc);

            hbmOld = (HBITMAP)SelectObject(hdcMem, hPlotBM);
            GetObject(hPlotBM, sizeof(bm), &bm);
            BitBlt(hdc, pxPlotXStart, pxPlotYStart, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hPlotSeparatorBM);
            GetObject(hPlotSeparatorBM, sizeof(bm), &bm);
            BitBlt(hdc, pxPlotSeparatorXStart, pxPlotYStart, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return TRUE;
            break;
        case WM_DESTROY:
            DeleteObject(hPlotSeparatorBM);
            DeleteObject(hPlotBM);
            PostQuitMessage(0);
            return TRUE;
        case WM_CLOSE:
            Cleanup(hWnd);
            return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: Cleanup
//
// Purpose: Handle resource cleanup as a common method to be called from various paths of program
//          exit.
//
// Parameters: hWnd: Handle to the main window.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void Cleanup( HWND hWnd )
{
    if (pScopeSelector->GetSelectedScope())
    {
        StoreSettings();
        pSettings->WriteScopeSettings();
    }
    pSettings->WriteApplicationSettings();
    delete fraPlotter;
    delete psFRA;
    delete pSettings;
    delete pScopeSelector;
    DestroyWindow (hWnd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: LoadControlsData
//
// Purpose: Load data into the controls, specific to the application and scope (gotten from the global
//          setting object)
//
// Parameters: hDlg: handle to the main window (which is a dialog)
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL LoadControlsData(HWND hDlg)
{
    PicoScope* pScope;

    TCHAR attenText[numAttens][6] =
    {
        TEXT("1x"), TEXT("10x"), TEXT("20x"), TEXT("100x"), TEXT("200x"), TEXT("1000x")
    };

    vector<wstring> couplingText;

    if (NULL != (pScope = pScopeSelector->GetSelectedScope()))
    {
        pScope->GetAvailableCouplings(couplingText);
    }
    else
    {
        couplingText.resize(2);
        couplingText[0] = TEXT("AC");
        couplingText[1] = TEXT("DC");
    }

    wstring chanText;

    HWND hndCB;
    HWND hndCtrl;

    // Load the Combo-boxes with entries.
    hndCB = GetDlgItem( hDlg, IDC_INPUT_CHANNEL );
    ComboBox_ResetContent( hndCB );
    for (int k = 0; k < pSettings->GetNumChannels(); k++)
    {
        chanText = L'A' + (wchar_t)k;
        // Add string to combobox.
        ComboBox_AddString( hndCB, chanText.c_str() );
        chanText[0]++;
    }
    SendMessage(hndCB, CB_SETCURSEL, (WPARAM)(pSettings->GetInputChannel()), (LPARAM)0);

    hndCB = GetDlgItem( hDlg, IDC_OUTPUT_CHANNEL );
    ComboBox_ResetContent( hndCB );
    for (int k = 0; k < pSettings->GetNumChannels(); k++)
    {
        chanText = L'A' + (wchar_t)k;
        // Add string to combobox.
        ComboBox_AddString( hndCB, chanText.c_str() );
        chanText[0]++;
    }
    SendMessage(hndCB, CB_SETCURSEL, (WPARAM)(pSettings->GetOutputChannel()), (LPARAM)0);

    hndCB = GetDlgItem( hDlg, IDC_INPUT_ATTEN );
    ComboBox_ResetContent( hndCB );
    for (int k = 0; k < numAttens; k++)
    {
        // Add string to combobox.
        ComboBox_AddString( hndCB, attenText[k] );
    }
    SendMessage(hndCB, CB_SETCURSEL, (WPARAM)(pSettings->GetInputAttenuation()), (LPARAM)0);

    hndCB = GetDlgItem( hDlg, IDC_OUTPUT_ATTEN );
    ComboBox_ResetContent( hndCB );
    for (int k = 0; k < numAttens; k++)
    {
        // Add string to combobox.
        ComboBox_AddString( hndCB, attenText[k] );
    }
    SendMessage(hndCB, CB_SETCURSEL, (WPARAM)(pSettings->GetOutputAttenuation()), (LPARAM)0);

    hndCB = GetDlgItem( hDlg, IDC_INPUT_COUPLING );
    ComboBox_ResetContent( hndCB );
    for (uint8_t k = 0; k < couplingText.size(); k++)
    {
        // Add string to combobox.
        ComboBox_AddString( hndCB, couplingText[k].c_str() );
    }
    SendMessage(hndCB, CB_SETCURSEL, (WPARAM)(pSettings->GetInputCoupling()), (LPARAM)0);

    hndCB = GetDlgItem( hDlg, IDC_OUTPUT_COUPLING );
    ComboBox_ResetContent( hndCB );
    for (uint8_t k = 0; k < couplingText.size(); k++)
    {
        // Add string to combobox.
        ComboBox_AddString( hndCB, couplingText[k].c_str() );
    }
    SendMessage(hndCB, CB_SETCURSEL, (WPARAM)(pSettings->GetOutputCoupling()), (LPARAM)0);

    hndCtrl = GetDlgItem( hDlg, IDC_INPUT_DC_OFFS );
    Edit_LimitText( hndCtrl, 16 );
    Edit_SetText( hndCtrl, pSettings->GetInputDcOffset().c_str() );

    hndCtrl = GetDlgItem( hDlg, IDC_OUTPUT_DC_OFFS );
    Edit_LimitText( hndCtrl, 16 );
    Edit_SetText( hndCtrl, pSettings->GetOutputDcOffset().c_str() );

    hndCtrl = GetDlgItem( hDlg, IDC_INPUT_SIGNAL_VPP );
    Edit_LimitText( hndCtrl, 16 );
    Edit_SetText( hndCtrl, pSettings->GetInputSignalVpp().c_str() );

    hndCtrl = GetDlgItem( hDlg, IDC_FRA_START_FREQ );
    Edit_LimitText( hndCtrl, 16 );
    Edit_SetText( hndCtrl, pSettings->GetStartFreq().c_str() );

    hndCtrl = GetDlgItem( hDlg, IDC_FRA_STOP_FREQ );
    Edit_LimitText( hndCtrl, 16 );
    Edit_SetText( hndCtrl, pSettings->GetStopFreq().c_str() );

    hndCtrl = GetDlgItem( hDlg, IDC_FRA_STEPS_PER_DECADE );
    Edit_LimitText( hndCtrl, 3 );
    Edit_SetText( hndCtrl, pSettings->GetStepsPerDecade().c_str() );

    if (pSettings->GetAutoAxes())
    {
        hndCtrl = GetDlgItem( hDlg, IDC_FRA_AUTO_AXES );
    }
    else
    {
        hndCtrl = GetDlgItem( hDlg, IDC_FRA_SAVED_AXES );
    }
    Button_SetCheck( hndCtrl, true );

    hndCtrl = GetDlgItem( hDlg, IDC_FRA_DRAW_GAIN );
    Button_SetCheck( hndCtrl, pSettings->GetPlotGain() );

    hndCtrl = GetDlgItem( hDlg, IDC_FRA_DRAW_PHASE );
    Button_SetCheck( hndCtrl, pSettings->GetPlotPhase() );

    hndCtrl = GetDlgItem( hDlg, IDC_FRA_UNWRAP_PHASE );
    Button_SetCheck( hndCtrl, pSettings->GetPlotUnwrappedPhase() );

    if (!pSettings->GetPlotPhase())
    {
        Button_Enable( hndCtrl, false );
    }

    if (pSettings->GetPlotGain() && pSettings->GetPlotPhase() && !pSettings->GetPlotUnwrappedPhase())
    {
        hndCtrl = GetDlgItem(hDlg, IDC_FRA_DRAW_GM);
        Button_Enable( hndCtrl, true );
        Button_SetCheck( hndCtrl, pSettings->GetPlotGainMargin() );
        hndCtrl = GetDlgItem(hDlg, IDC_FRA_DRAW_PM);
        Button_Enable( hndCtrl, true );
        Button_SetCheck( hndCtrl, pSettings->GetPlotPhaseMargin() );
    }
    else
    {
        hndCtrl = GetDlgItem(hDlg, IDC_FRA_DRAW_GM);
        Button_SetCheck( hndCtrl, false );
        Button_Enable( hndCtrl, false );
        hndCtrl = GetDlgItem(hDlg, IDC_FRA_DRAW_PM);
        Button_SetCheck( hndCtrl, false );
        Button_Enable( hndCtrl, false );
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: InitializeControls
//
// Purpose: Initialize main window controls with non-settings specific data.
//
// Parameters: hDlg: handle to the main window (which is a dialog)
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitializeControls(HWND hDlg)
{
    HWND hndCtrl;

    // Size the log
    hndCtrl = GetDlgItem( hDlg, IDC_LOG );
    SendMessage( hndCtrl, EM_SETLIMITTEXT, 8388608, 0); // 8M

    //// Determine where the the plot and separator will go.

    // Get margin pixels from DLUs
    RECT marginRect = {0, 0, 6, 12}; // Use right for small margin (6), and bottom for large margin (12)
    MapDialogRect(hDlg, &marginRect);

    // Find the rightmost edge of the controls
    hndCtrl = GetDlgItem( hDlg, IDC_RIGHTMOST_CTL );
    RECT rightmostCtlRect;
    GetWindowRect( hndCtrl, &rightmostCtlRect );
    POINT point = {rightmostCtlRect.right, rightmostCtlRect.bottom};
    ScreenToClient( hDlg, &point );
    pxPlotXStart = point.x + marginRect.bottom;

    pxPlotYStart = marginRect.right;

    pxPlotSeparatorXStart = point.x + (marginRect.bottom / 2);

    // Find the size of the dialog box on this system
    RECT dlgClientRect;
    GetClientRect( hDlg, &dlgClientRect );

    pxPlotWidth = dlgClientRect.right - pxPlotXStart - marginRect.right;
    pxPlotHeight = dlgClientRect.bottom - pxPlotYStart - marginRect.right;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ExecuteFRA
//
// Purpose: Carries out execution of the FRA.  
//
// Parameters:
//
// Notes: Is a thread function, which supports cancellation.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI ExecuteFRA(LPVOID lpdwThreadParam)
{
    int inputChannel = 0;
    int inputChannelCoupling = 0;
    int inputChannelAttenuation = 0;
    double inputDcOffset = 0.0;
    int outputChannel = 0;
    int outputChannelCoupling = 0;
    int outputChannelAttenuation = 0;
    double outputDcOffset = 0.0;
    double signalVpp = 0.0;
    double startFreq = 0;
    double stopFreq = 0;
    int stepsPerDecade = 0;

    DWORD dwWaitResult;

    for (;;)
    {
        // Place these here because this is where we'll always return whether the FRA executes completely or not.
        EnableAllMenus();
        if (!ResetEvent( hExecuteFraEvent ))
        {
            LogMessage( L"Fatal error: Failed to reset FRA execution start event" );
            return -1;
        }

        dwWaitResult = WaitForSingleObject( hExecuteFraEvent, INFINITE );

        if (dwWaitResult == WAIT_OBJECT_0)
        {
#if defined(TEST_PLOTTING)
            StoreSettings();
            (void)GeneratePlot(true);
#else
            if (NULL == pScopeSelector->GetSelectedScope()) // Scope not created
            {
                LogMessage( L"Error: Device not initialized." );
                continue;
            }
            else if (!(pScopeSelector->GetSelectedScope()->IsCompatible()))
            {
                LogMessage( L"Error: Selected scope is not compatible." );
                continue;
            }

            if (!ValidateSettings())
            {
                LogMessage( L"Error: Invalid inputs." );
                continue;
            }

            DisableAllMenus();

            StoreSettings();

            inputChannel = pSettings->GetInputChannel();
            inputChannelCoupling = pSettings->GetInputCoupling();
            inputChannelAttenuation = pSettings->GetInputAttenuation();
            outputChannel = pSettings->GetOutputChannel();
            outputChannelCoupling = pSettings->GetOutputCoupling();
            outputChannelAttenuation = pSettings->GetOutputAttenuation();
            inputDcOffset = pSettings->GetInputDcOffsetAsDouble();
            outputDcOffset = pSettings->GetOutputDcOffsetAsDouble();
            signalVpp = pSettings->GetInputSignalVppAsDouble();
            startFreq = pSettings->GetStartFreqAsDouble();
            stopFreq = pSettings->GetStopFreqAsDouble();
            stepsPerDecade = pSettings->GetStepsPerDecadeAsInt();

            if (false == psFRA -> SetupChannels( inputChannel, inputChannelCoupling, inputChannelAttenuation, inputDcOffset,
                                                 outputChannel, outputChannelCoupling, outputChannelAttenuation, outputDcOffset,
                                                 signalVpp ))
            {
                continue;
            }

            if (false == psFRA -> ExecuteFRA( startFreq, stopFreq, stepsPerDecade ))
            {
                continue;
            }

            if (!GeneratePlot(true))
            {
                continue;
            }
#endif
            RepaintPlot();
            initialPlot = true;
        }
        else
        {
            LogMessage( L"Fatal error: Invalid result from waiting on FRA execution start event" );
            return -1;
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: GeneratePlot
//
// Purpose: Common function to generate the plot
//
// Parameters: rescale - whether to re-scale the plot to initial settings (auto or saved).
//
// Notes: Does error handling and can write to the log on error.
//        Assumes UI settingshave been stored to aplication settings
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool GeneratePlot(bool rescale)
{
    tuple<bool,double,double> freqAxisScale, gainAxisScale, phaseAxisScale;
    tuple<double,uint8_t,bool,bool> freqAxisIntervals, gainAxisIntervals, phaseAxisIntervals;
    bool gainMasterIntervals, phaseMasterIntervals;
    int numSteps;
    double *freqsLogHz, *phasesDeg, *unwrappedPhasesDeg, *gainsDb, *phases;

    if (rescale)
    {
        if (pSettings->GetAutoAxes())
        {
            // Set scales and intervals for automatic axes/grid determination
            freqAxisScale = gainAxisScale = phaseAxisScale = tuple<bool,double,double>(true, 0.0, 0.0);
            freqAxisIntervals = gainAxisIntervals = phaseAxisIntervals = tuple<double,uint8_t,bool,bool>(0.0, 0, true, true);

            // Default to gain being the master axes
            if (pSettings->GetPlotGain())
            {
                gainMasterIntervals = true;
                phaseMasterIntervals = false;
            }
            else
            {
                gainMasterIntervals = false;
                phaseMasterIntervals = true;
            }
        }
        else
        {
            freqAxisScale = pSettings->GetFreqScale();
            gainAxisScale = pSettings->GetGainScale();
            phaseAxisScale = pSettings->GetPhaseScale();
            freqAxisIntervals = pSettings->GetFreqIntervals();
            gainAxisIntervals = pSettings->GetGainIntervals();
            phaseAxisIntervals = pSettings->GetPhaseIntervals();
            gainMasterIntervals = pSettings->GetGainMasterIntervals();
            phaseMasterIntervals = pSettings->GetPhaseMasterIntervals();
        }
    }

#if !defined(TEST_PLOTTING)
    psFRA->GetResults( &numSteps, &freqsLogHz, &gainsDb, &phasesDeg, &unwrappedPhasesDeg );
#else
    numSteps = testPlotN;
    freqsLogHz = testPlotFreqs;
    gainsDb = testPlotGains;
    phasesDeg = testPlotPhases;
    unwrappedPhasesDeg = testPlotUnwrappedPhases;
#endif
    phases = pSettings->GetPlotUnwrappedPhase() ? unwrappedPhasesDeg : phasesDeg;

    try
    {
        if (rescale)
        {
            fraPlotter -> SetPlotSettings( pSettings->GetPlotGain(), pSettings->GetPlotPhase(), pSettings->GetPlotGainMargin(), pSettings->GetPlotPhaseMargin(),
                                           pSettings->GetGainMarginPhaseCrossover(), false );
            fraPlotter -> PlotFRA( freqsLogHz, gainsDb, phases, numSteps,
                                   freqAxisScale, gainAxisScale, phaseAxisScale,
                                   freqAxisIntervals, gainAxisIntervals, phaseAxisIntervals,
                                   gainMasterIntervals, phaseMasterIntervals );
        }
        else
        {
            fraPlotter -> SetPlotData( freqsLogHz, gainsDb, phases, numSteps, false );
            fraPlotter -> SetPlotSettings( pSettings->GetPlotGain(), pSettings->GetPlotPhase(), pSettings->GetPlotGainMargin(), pSettings->GetPlotPhaseMargin(),
                                           pSettings->GetGainMarginPhaseCrossover(), true );
        }
    }
    catch (runtime_error e)
    {
        wstringstream wss;
        wss << e.what();
        LogMessage( wss.str() );
        return false;
    }

    return true;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: RepaintsPlot
//
// Purpose: Common function to re-draw the generated plot to the application window
//
// Parameters: None
//
// Notes: Does error handling and can write to the log on error.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void RepaintPlot(void)
{
    unique_ptr<uint8_t[]> buffer;

    try
    {
        buffer = fraPlotter->GetScreenBitmapPlot32BppBGRA();
    }
    catch (runtime_error e)
    {
        wstringstream wss;
        wss << e.what();
        LogMessage( wss.str() );
        return;
    }

    if (DeleteObject( hPlotBM ))
    {
        if (NULL != (hPlotBM = CreateBitmap(pxPlotWidth, pxPlotHeight, 1, 32, buffer.get())))
        {
            RECT invalidRect = { pxPlotXStart, pxPlotYStart, pxPlotXStart + pxPlotWidth, pxPlotYStart + pxPlotHeight };
            if (InvalidateRect( hMainWnd, &invalidRect, true ))
            {
                plotAvailable = true;
            }
            else
            {
                LogMessage( L"Error: InvalidateRect failed while painting plot. Plot image may be stale." );
            }
        }
        else
        {
            LogMessage( L"Error: CreateBitmap failed while painting plot. Plot image may be stale." );
        }
    }
    else
    {
        LogMessage( L"Error: DeleteObject failed while painting plot. Plot image may be stale." );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ValidateSettings
//
// Purpose: Check whether the users provided setting are valid.
//
// Parameters: [out] return: true if valid, false otherwise
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ValidateSettings(void)
{
    auto retVal = true;

    HWND hndCtrl;
    TCHAR editText[16];
    wstringstream wss;
    double inputDcOffset;
    double outputDcOffset;
    double signalVpp;
    double startFreq;
    double stopFreq;
    tuple <bool, double, double> freqScale;
    int16_t stepsPerDecade;
    bool validStartFreq = false;
    bool validStopFreq = false;

    hndCtrl = GetDlgItem( hMainWnd, IDC_INPUT_DC_OFFS );
    Edit_GetText( hndCtrl, editText, 16 );
    if (!WStringToDouble(editText, inputDcOffset))
    {
        LogMessage( L"Input DC offset is not a valid number." );
        retVal = false;
    }

    hndCtrl = GetDlgItem( hMainWnd, IDC_OUTPUT_DC_OFFS );
    Edit_GetText( hndCtrl, editText, 16 );
    if (!WStringToDouble(editText, outputDcOffset))
    {
        LogMessage( L"Output DC offset is not a valid number." );
        retVal = false;
    }

    hndCtrl = GetDlgItem( hMainWnd, IDC_INPUT_SIGNAL_VPP );
    Edit_GetText( hndCtrl, editText, 16 );
    if (!WStringToDouble(editText, signalVpp))
    {
        LogMessage( L"Input signal Vpp is not a valid number." );
        retVal = false;
    }
    else if (signalVpp <= 0.0)
    {
        LogMessage( L"Input signal Vpp must be > 0.0" );
        retVal = false;
    }
    else
    {
        double maxSignalVpp = pScopeSelector->GetSelectedScope()->GetMaxFuncGenVpp();
        if (signalVpp > maxSignalVpp)
        {
            wstringstream wss;
            wss << L"Input signal Vpp must be <= " << fixed << setprecision(1) << maxSignalVpp;
            LogMessage( wss.str() );
            retVal = false;
        }
    }

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_START_FREQ );
    Edit_GetText( hndCtrl, editText, 16 );
    if (!WStringToDouble(editText, startFreq))
    {
        LogMessage( L"Start frequency is not a valid number." );
        retVal = false;
    }
    else
    {
        double minFreq = psFRA->GetMinFrequency();
        if (startFreq < minFreq)
        {
            wstringstream wss;
            wss << L"Start frequency must be >= " << minFreq;
            LogMessage( wss.str() );
            retVal = false;
        }
        else
        {
            validStartFreq = true;
        }
    }

    // Check to make sure that manually set frequency axis limts are not excessively
    // small; in particlar also guarding against 0.0 (without comparing to 0.0).
    // This could happen if the user manually edits the settings file.
    // If they are too small, just adjust them based on scope capabilities.
    // This is important because we don't want the plotting routines trying
    // to take a log of 0.0.  
    freqScale = pSettings->GetFreqScale();
    if (!get<0>(freqScale)) // If not autoscale
    {
        if (get<1>(freqScale) < psFRA->GetMinFrequency())
        {
            wstringstream wss;
            get<1>(freqScale) = psFRA->GetMinFrequency();
            pSettings->SetFreqScale(freqScale);
            wss << L"Warning: Frequency axis minimum too small; adjusting to: " << get<1>(freqScale);
            LogMessage( wss.str() );
        }
        if (get<2>(freqScale) < psFRA->GetMinFrequency())
        {
            wstringstream wss;
            get<2>(freqScale) = pScopeSelector->GetSelectedScope()->GetMaxFuncGenFreq();
            pSettings->SetFreqScale(freqScale);
            wss << L"Warning: Frequency axis maximum too small; adjusting to: " << get<2>(freqScale);
            LogMessage( wss.str() );
        }
    }

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_STOP_FREQ );
    Edit_GetText( hndCtrl, editText, 16 );
    if (!WStringToDouble(editText, stopFreq))
    {
        LogMessage( L"Stop frequency is not a valid number." );
        retVal = false;
    }
    else
    {
        double maxFreq = pScopeSelector->GetSelectedScope()->GetMaxFuncGenFreq();
        if (stopFreq > maxFreq)
        {
            wstringstream wss;
            wss << L"Stop frequency must be <= " << fixed << setprecision(1) << maxFreq;
            LogMessage( wss.str() );
            retVal = false;
        }
        else
        {
            validStopFreq = true;
        }
    }

    if (validStartFreq && validStopFreq && startFreq >= stopFreq)
    {
        LogMessage( L"Stop frequency must be > start frequency" );
        retVal = false;
    }

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_STEPS_PER_DECADE );
    Edit_GetText( hndCtrl, editText, 16 );
    if (!WStringToInt16(editText, stepsPerDecade))
    {
        LogMessage( L"Steps per decade is not a valid number." );
        retVal = false;
    }
    else if (stepsPerDecade < 1)
    {
        LogMessage( L"Steps per decade must be >= 1." );
        retVal = false;
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: StoreSettings
//
// Purpose: Store user settings back to the global settings object.
//
// Parameters: [out] return: currently not used, always true
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool StoreSettings(void)
{
    HWND hndCtrl;
    TCHAR editText[16];

    // Channel Settings
    hndCtrl = GetDlgItem( hMainWnd, IDC_INPUT_CHANNEL );
    pSettings -> SetInputChannel(ComboBox_GetCurSel( hndCtrl ));

    hndCtrl = GetDlgItem( hMainWnd, IDC_INPUT_COUPLING );
    pSettings -> SetInputCoupling(ComboBox_GetCurSel( hndCtrl ));

    hndCtrl = GetDlgItem( hMainWnd, IDC_INPUT_ATTEN );
    pSettings -> SetInputAttenuation(ComboBox_GetCurSel( hndCtrl ));

    hndCtrl = GetDlgItem( hMainWnd, IDC_OUTPUT_CHANNEL );
    pSettings -> SetOutputChannel(ComboBox_GetCurSel( hndCtrl ));

    hndCtrl = GetDlgItem( hMainWnd, IDC_OUTPUT_COUPLING );
    pSettings -> SetOutputCoupling(ComboBox_GetCurSel( hndCtrl ));

    hndCtrl = GetDlgItem( hMainWnd, IDC_OUTPUT_ATTEN );
    pSettings -> SetOutputAttenuation(ComboBox_GetCurSel( hndCtrl ));

    hndCtrl = GetDlgItem( hMainWnd, IDC_INPUT_DC_OFFS );
    Edit_GetText( hndCtrl, editText, 16 );
    pSettings -> SetInputDcOffset( editText );

    hndCtrl = GetDlgItem( hMainWnd, IDC_OUTPUT_DC_OFFS );
    Edit_GetText( hndCtrl, editText, 16 );
    pSettings -> SetOutputDcOffset( editText );

    hndCtrl = GetDlgItem( hMainWnd, IDC_INPUT_SIGNAL_VPP );
    Edit_GetText( hndCtrl, editText, 16 );
    pSettings -> SetInputSignalVpp( editText );

    // FRA inputs
    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_START_FREQ );
    Edit_GetText( hndCtrl, editText, 16 );
    pSettings->SetStartFrequency( editText );

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_STOP_FREQ );
    Edit_GetText( hndCtrl, editText, 16 );
    pSettings->SetStopFrequency( editText );

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_STEPS_PER_DECADE );
    Edit_GetText( hndCtrl, editText, 16 );
    pSettings->SetStepsPerDecade( editText );

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_AUTO_AXES );
    pSettings->SetAutoAxes(Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_DRAW_GAIN );
    pSettings->SetPlotGain(Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_DRAW_PHASE );
    pSettings->SetPlotPhase(Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_DRAW_GM );
    pSettings->SetPlotGainMargin(Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_DRAW_PM );
    pSettings->SetPlotPhaseMargin(Button_GetCheck( hndCtrl ) == BST_CHECKED);

    hndCtrl = GetDlgItem( hMainWnd, IDC_FRA_UNWRAP_PHASE );
    pSettings->SetPlotUnwrappedPhase(Button_GetCheck( hndCtrl ) == BST_CHECKED);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: FraStatusCallback
//
// Purpose: Callback, provided as a means to indicate status of various operations.  Status is 
//          interpreted and written to the log.  Also acts as part of the messaging to support
//          interactive auto-ranging
//
// Parameters: [in] fraStatusMsg: The status
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool FraStatusCallback( FRA_STATUS_MESSAGE_T& fraStatusMsg )
{
    if (fraStatusMsg.status == FRA_STATUS_PROGRESS)
    {
        HWND hndCtrl;
        TCHAR szStatus[64];

        wsprintf( szStatus, L"%d of %d steps complete",
                  fraStatusMsg.statusData.progress.stepsComplete,
                  fraStatusMsg.statusData.progress.numSteps );

        hndCtrl = GetDlgItem( hMainWnd, IDC_STATUS_TEXT );
        Edit_SetText( hndCtrl, szStatus );
    }
    else if (fraStatusMsg.status == FRA_STATUS_CANCELED)
    {
        HWND hndCtrl;
        TCHAR szStatus[64];

        wsprintf( szStatus, L"Canceled at %d of %d steps complete",
                  fraStatusMsg.statusData.cancelPoint.stepsComplete,
                  fraStatusMsg.statusData.cancelPoint.numSteps );

        hndCtrl = GetDlgItem( hMainWnd, IDC_STATUS_TEXT );
        Edit_SetText( hndCtrl, szStatus );
    }
    else if (fraStatusMsg.status == FRA_STATUS_FATAL_ERROR)
    {
        HWND hndCtrl;
        TCHAR szStatus[64];

        wsprintf( szStatus, L"Fatal error at %d of %d steps complete",
                  fraStatusMsg.statusData.progress.stepsComplete,
                  fraStatusMsg.statusData.progress.numSteps );

        hndCtrl = GetDlgItem( hMainWnd, IDC_STATUS_TEXT );
        Edit_SetText( hndCtrl, szStatus );

        hndCtrl = GetDlgItem( hMainWnd, IDC_LOG );
        int insertPoint = GetWindowTextLength(hndCtrl);
        Edit_SetSel( hndCtrl, insertPoint, insertPoint );
        fraStatusMsg.statusText.append(L"\r\n");
        Edit_ReplaceSel( hndCtrl, fraStatusMsg.statusText.c_str() );
        Edit_SetSel( hndCtrl, -1, -1 );
        Edit_ScrollCaret( hndCtrl );
    }
    else if (fraStatusMsg.status == FRA_STATUS_MESSAGE)
    {
        HWND hndCtrl;
        hndCtrl = GetDlgItem( hMainWnd, IDC_LOG );
        int insertPoint = GetWindowTextLength(hndCtrl);
        Edit_SetSel( hndCtrl, insertPoint, insertPoint );
        fraStatusMsg.statusText.append(L"\r\n");
        Edit_ReplaceSel( hndCtrl, fraStatusMsg.statusText.c_str() );
        Edit_SetSel( hndCtrl, -1, -1 );
        Edit_ScrollCaret( hndCtrl );
    }
    else if (fraStatusMsg.status == FRA_STATUS_AUTORANGE_LIMIT)
    {
        int response = MessageBoxEx( hMainWnd, L"Reached autorange limit.  Continue?", L"Autorange feedback", MB_OKCANCEL, 0 );

        if (IDCANCEL == response)
        {
            fraStatusMsg.responseData.proceed = false;
        }
        else if (IDOK == response)
        {
            fraStatusMsg.responseData.proceed = true;
        }

    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: LogMessage
//
// Purpose: Provide a simpler functionality than FraStatusCallback for local needs to simply display 
//          log messages
//
// Parameters: statusMessage: string to log/display
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void LogMessage( const wstring statusMessage )
{
    HWND hndCtrl;
    hndCtrl = GetDlgItem( hMainWnd, IDC_LOG );
    int insertPoint = GetWindowTextLength(hndCtrl);
    Edit_SetSel( hndCtrl, insertPoint, insertPoint );
    wstring msg = statusMessage;
    msg.append(L"\r\n");
    Edit_ReplaceSel( hndCtrl, msg.c_str() );
    Edit_SetSel( hndCtrl, -1, -1 );
    Edit_ScrollCaret( hndCtrl );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: CopyLog
//
// Purpose: Copies the log to the clipboard
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void CopyLog(void)
{
    HWND hndCtrl;
    hndCtrl = GetDlgItem( hMainWnd, IDC_LOG );
    int length = GetWindowTextLength(hndCtrl);
    Edit_SetSel( hndCtrl, 0, length );
    SendMessage( hndCtrl, WM_COPY, 0, 0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ClearLog
//
// Purpose: Erases the log from the display
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void ClearLog(void)
{
    HWND hndCtrl;
    hndCtrl = GetDlgItem( hMainWnd, IDC_LOG );
    Edit_SetText( hndCtrl, L"" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: About
//
// Purpose: Message handler for Help->About box.
//
// Parameters: See Windows programming API information
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND hAppIdText = GetDlgItem( hDlg, IDC_APPID );
            wstringstream AppIdSS;
            AppIdSS << appNameString << L", Version " << appVersionString;
            Static_SetText( hAppIdText, AppIdSS.str().c_str() );
            return (INT_PTR)TRUE;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ScopeSelectHandler
//
// Purpose: Message handler for Scope selection dialog box.
//
// Parameters: See Windows programming API information
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ScopeSelectHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    bool scopeAvail = false;
    LVCOLUMN listViewCol;
    LVITEM listViewItem;

    // Used to get text width pixels from DLUs; 220 (minus margin = 210) split three ways
    RECT textWidthRect = {50, 80, 80, 0}; // Use in ascending order for text width

    switch (message)
    {
        case WM_INITDIALOG:

            vector<AvailableScopeDescription_T>* pScopes;
            HWND hndLvCtrl;
            hndLvCtrl = GetDlgItem( hDlg, IDC_SCOPE_SELECT_LIST );

            ListView_SetExtendedListViewStyle( hndLvCtrl, ListView_GetExtendedListViewStyle(hndLvCtrl) | LVS_EX_FULLROWSELECT );

            MapDialogRect(hDlg, &textWidthRect);

            listViewCol.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;

            listViewCol.pszText = L"Family";
            listViewCol.fmt = LVCFMT_LEFT;
            listViewCol.cx = textWidthRect.left;
            listViewCol.iSubItem = 0;
            ListView_InsertColumn(hndLvCtrl, 0, &listViewCol );

            listViewCol.pszText = L"S/N";
            listViewCol.fmt = LVCFMT_LEFT;
            listViewCol.cx = textWidthRect.top;
            listViewCol.iSubItem = 1;
            ListView_InsertColumn(hndLvCtrl, 1, &listViewCol );

            listViewCol.pszText = L"Status";
            listViewCol.fmt = LVCFMT_LEFT;
            listViewCol.cx = textWidthRect.right;
            listViewCol.iSubItem = 2;
            ListView_InsertColumn(hndLvCtrl, 2, &listViewCol );

            pScopes = (vector<AvailableScopeDescription_T>*)lParam;

            listViewItem.mask = LVIF_TEXT;
            for (uint8_t idx = 0; idx < pScopes->size(); idx++)
            {
                listViewItem.iItem = idx;

                listViewItem.pszText = (LPWSTR)(*pScopes)[idx].wFamilyName.c_str();
                listViewItem.iSubItem = 0;
                ListView_InsertItem( hndLvCtrl, &listViewItem );

                ListView_SetItemText(hndLvCtrl, idx, 1, (LPWSTR)(*pScopes)[idx].wSerialNumber.c_str());

                if ((*pScopes)[idx].connected)
                {
                    ListView_SetItemText( hndLvCtrl, idx, 2, L"Connected" );
                }
                else
                {
                    scopeAvail = true;
                    ListView_SetItemText( hndLvCtrl, idx, 2, L"Available" );
                }
            }

            ListView_SetItemState( hndLvCtrl, 0, LVIS_SELECTED, LVIS_SELECTED );

            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCLOSE)
            {
                int index;
                HWND hndLvCtrl;
                hndLvCtrl = GetDlgItem( hDlg, IDC_SCOPE_SELECT_LIST );
                index = ListView_GetNextItem( hndLvCtrl, -1, LVNI_SELECTED );
                EndDialog(hDlg, index<<16 | LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
        case WM_NOTIFY:
            int itemSelected;
            NMHDR* nmhdr = (NMHDR*)lParam;
            if ( nmhdr->code == LVN_ITEMCHANGED )
            {
                vector<AvailableScopeDescription_T> scopes;
                HWND hndConnectButton;
                hndConnectButton = GetDlgItem( hDlg, IDOK );

                NMLISTVIEW *nmlv = (NMLISTVIEW*)lParam;
                if (nmlv->uNewState & LVIS_SELECTED)
                {
                    itemSelected = nmlv->iItem;

                    pScopeSelector->GetAvailableScopes(scopes, false);

                    // If the scope selected is already connected, disable the Connect button
                    if (scopes[itemSelected].connected)
                    {
                        Button_Enable( hndConnectButton, FALSE );
                    }
                    else
                    {
                        Button_Enable( hndConnectButton, TRUE );
                    }
                }
                else // New state for some item is unselected
                {
                    if (-1 == ListView_GetNextItem( nmhdr->hwndFrom, -1, LVNI_SELECTED ))
                    {
                        Button_Enable( hndConnectButton, FALSE );
                    }
                }
            }
    }
    return (INT_PTR)FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SaveRawData
//
// Purpose: Saves the raw data results (frequencies, gain, phase) to a comma separated data file
//
// Parameters: dataFilePath: path to the data file.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void SaveRawData( wstring dataFilePath )
{
    int numSteps;
    double *freqsLogHz, *phasesDeg, *unwrappedPhasesDeg, *gainsDb, *phases;

    ofstream dataFileOutputStream;

    psFRA->GetResults( &numSteps, &freqsLogHz, &gainsDb, &phasesDeg, &unwrappedPhasesDeg );

    if (numSteps == 0)
    {
        MessageBox( 0, L"No data is available to export.", L"Error", MB_OK );
    }
    else
    {
        dataFileOutputStream.open( dataFilePath.c_str(), ios::out );

        if (dataFileOutputStream)
        {
            phases = pSettings->GetPlotUnwrappedPhase() ? unwrappedPhasesDeg : phasesDeg;
            dataFileOutputStream << "Frequency Log(Hz), Gain (dB), Phase (deg)\n";
            dataFileOutputStream.precision(numeric_limits<double>::digits10);
            for (int idx = 0; idx < numSteps; idx++)
            {
                dataFileOutputStream << freqsLogHz[idx] << ", " << gainsDb[idx] << ", " << phases[idx] << "\n";
            }

            dataFileOutputStream.close();

            wstring message = L"Exported data to file: " + dataFilePath;
            LogMessage(message);
        }
        else
        {
            wstring message = L"Could not write data file: " + dataFilePath + L".  Check that the directory exists and has proper permissions set.";
            MessageBox( 0, message.c_str(), L"Error", MB_OK );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: SavePlotImageFile
//
// Purpose: Saves the current plot image to a PNG image file
//
// Parameters: dataFilePath: path to the image file.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void SavePlotImageFile( wstring dataFilePath )
{
    ofstream dataFileOutputStream;
    size_t bufferSize;
    unique_ptr<uint8_t[]> buffer;

    if ( !fraPlotter->PlotDataAvailable() )
    {
        MessageBox( 0, L"No plot is available to save.", L"Error", MB_OK );
    }
    else
    {
        dataFileOutputStream.open( dataFilePath.c_str(), ios::out | ios::binary );

        if (dataFileOutputStream)
        {
            try
            {
                buffer = fraPlotter->GetPNGBitmapPlot(&bufferSize);
            }
            catch (runtime_error e)
            {
                wstringstream wss;
                wss << e.what();
                LogMessage( wss.str() );
                return;
            }

            if (buffer && bufferSize)
            {
                dataFileOutputStream.write( (const char*)buffer.get(), bufferSize );

                if (dataFileOutputStream.good())
                {
                    wstring message = L"Saved plot image to file: " + dataFilePath;
                    LogMessage(message);
                }
                else
                {
                    wstring message = L"Failed to write plot image file: " + dataFilePath + L".  Internal stream error.";
                    MessageBox( 0, message.c_str(), L"Error", MB_OK );
                }
            }
            else
            {
                wstring message = L"Failed to generate plot image data: " + dataFilePath + L".  Internal error.";
                MessageBox( 0, message.c_str(), L"Error", MB_OK );
            }
            dataFileOutputStream.close();
        }
        else
        {
            wstring message = L"Could not write plot image file: " + dataFilePath + L".  Check that the directory exists and has proper permissions set.";
            MessageBox( 0, message.c_str(), L"Error", MB_OK );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: [Enable|Disable]AllMenus
//
// Purpose: Enable/Disable the main window menus.  Used to disable the menus while the FRA is 
//          running.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void EnableAllMenus( void )
{
    HMENU hMenu = GetMenu( hMainWnd );
    int numMenuItems = GetMenuItemCount( hMenu );
    for (int i = 0; i < numMenuItems; i++)
    {
        EnableMenuItem( hMenu, i, MF_BYPOSITION | MF_ENABLED );
    }
    DrawMenuBar( hMainWnd );
}

void DisableAllMenus( void )
{
    HMENU hMenu = GetMenu( hMainWnd );
    int numMenuItems = GetMenuItemCount( hMenu );
    for (int i = 0; i < numMenuItems; i++)
    {
        EnableMenuItem( hMenu, i, MF_BYPOSITION | MF_GRAYED );
    }
    DrawMenuBar( hMainWnd );
}

#if defined(TEST_PLOTTING)
double testPlotFreqs[testPlotN] = {
1.0,
1.1,
1.2,
1.3,
1.4,
1.5,
1.6,
1.7,
1.8,
1.9,
2.0,
2.1,
2.2,
2.3,
2.4,
2.5,
2.6,
2.7,
2.8,
2.9,
3.0,
3.1,
3.2,
3.3,
3.4,
3.5,
3.6,
3.7,
3.8,
3.9,
4.0,
4.1,
4.2,
4.3,
4.4,
4.5,
4.6,
4.7,
4.8,
4.9,
5.0};

double testPlotGains[testPlotN] = 
{
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
-8.0,
-16.0,
-24.0,
-32.0,
-40.0,
-48.0,
-56.0,
-64.0,
-72.0,
-80.0,
-88.0,
-96.0,
-104.0,
-112.0,
-120.0,
-128.0,
-136.0,
-144.0,
-152.0,
-160.0
};

double testPlotPhases[testPlotN] = 
{
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
-18.0,
-36.0,
-54.0,
-72.0,
-90.0,
-108.0,
-126.0,
-144.0,
-162.0,
-180.0,
162.0,
144.0,
126.0,
108.0,
90.0,
72.0,
54.0,
36.0,
18.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0
};

double testPlotUnwrappedPhases[testPlotN] = 
{
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
0.0,
-18.0,
-36.0,
-54.0,
-72.0,
-90.0,
-108.0,
-126.0,
-144.0,
-162.0,
-180.0,
-198.0,
-216.0,
-234.0,
-252.0,
-270.0,
-288.0,
-306.0,
-324.0,
-342.0,
-360.0,
-360.0,
-360.0,
-360.0,
-360.0,
-360.0,
-360.0,
-360.0,
-360.0,
-360.0,
-360.0
};

#endif