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
// Module: ApplicationSettings.cpp
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ApplicationSettings.h"
#include <Shlobj.h>
#include <Shlwapi.h>
#include <sstream>
#include <iomanip>

#include <fstream>
#include <codecvt>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/exceptions.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::ApplicationSettings
//
// Purpose: Constructor
//
// Parameters: [in] _appDataFolder - Name of folder location where settings are stored.
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationSettings::ApplicationSettings( wstring _appDataFolder ) : appDataFolder(_appDataFolder)
{
    appSettingsOpened = false;
    appSettingsDirty = false;
    scopeSettingsOpened = false;
    scopeSettingsDirty = false;

    appDataFilename = appDataFolder + L"\\FRA4PicoScope\\Fra4PicoScopeSettings.xml";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::~ApplicationSettings
//
// Purpose: Destructor
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationSettings::~ApplicationSettings(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::ReadApplicationSettings
//
// Purpose: Reads the application settings from the application settings file
//
// Parameters: [out] return - Whether the function was successful
//
// Notes: Settings are read into a Boost property tree, where they will remain for later retrieval
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationSettings::ReadApplicationSettings( void )
{
    bool retVal = true;
    ifstream settingsFileInputStream;

    try
    {
        settingsFileInputStream.open( appDataFilename.c_str(), ios::in );

        if (settingsFileInputStream)
        {
            read_xml(settingsFileInputStream, AppSettingsPropTree, xml_parser::trim_whitespace);
            appSettingsOpened = true;
            appSettingsDirty = false;
        }
        else
        {
            InitializeApplicationSettingsFile();
        }
    }
    catch( const ptree_error& pte )
    {
        UNREFERENCED_PARAMETER(pte);
        retVal = false;
        settingsFileInputStream.close();
    }

    settingsFileInputStream.close();
    return retVal;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::InitializeApplicationSettingsFile
//
// Purpose: Creates an initial settings file for application settings
//
// Parameters: [out] return - Whether the function was successful
//
// Notes: Used for creating the file for the first time.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationSettings::InitializeApplicationSettingsFile( void )
{
    bool retVal = true;
    ofstream settingsFileOutputStream;

    try
    {
        AppSettingsPropTree.clear();

        AppSettingsPropTree.put( "mostRecentScope.SN", string("None") );
        AppSettingsPropTree.put( "mostRecentScope.family", PS_NO_FAMILY );

        AppSettingsPropTree.put( "samplingMode", LOW_NOISE );

        AppSettingsPropTree.put( "plot.showGridmarks", false );
        AppSettingsPropTree.put( "plot.plotGainMargin", false );
        AppSettingsPropTree.put( "plot.plotPhaseMargin", false );
        AppSettingsPropTree.put( "plot.screenColor.background.red", 0 );
        AppSettingsPropTree.put( "plot.screenColor.background.green", 0 );
        AppSettingsPropTree.put( "plot.screenColor.background.blue", 0 );
        AppSettingsPropTree.put( "plot.screenColor.axesGridsLabels.red", 0 );
        AppSettingsPropTree.put( "plot.screenColor.axesGridsLabels.green", 0 );
        AppSettingsPropTree.put( "plot.screenColor.axesGridsLabels.blue", 0 );
        AppSettingsPropTree.put( "plot.screenColor.gainPlot.red", 0 );
        AppSettingsPropTree.put( "plot.screenColor.gainPlot.green", 0 );
        AppSettingsPropTree.put( "plot.screenColor.gainPlot.blue", 0 );
        AppSettingsPropTree.put( "plot.screenColor.phasePlot.red", 0 );
        AppSettingsPropTree.put( "plot.screenColor.phasePlot.green", 0 );
        AppSettingsPropTree.put( "plot.screenColor.phasePlot.blue", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.background.red", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.background.green", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.background.blue", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.axesGridsLabels.red", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.axesGridsLabels.green", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.axesGridsLabels.blue", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.gainPlot.red", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.gainPlot.green", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.gainPlot.blue", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.phasePlot.red", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.phasePlot.green", 0 );
        AppSettingsPropTree.put( "plot.savedImageFileColor.phasePlot.blue", 0 );

        AppSettingsPropTree.put( "diagnostics.logVerbosityLevel", 0 );
        AppSettingsPropTree.put( "diagnostics.timeDomainPlots", false );

        AppSettingsPropTree.put( "expert.purityLowerLimit", 0.80 ); // 80%
        AppSettingsPropTree.put( "expert.extraSettlingTimeMs", 0 );
        AppSettingsPropTree.put( "expert.autorangeTriesPerStep", 10 );
        AppSettingsPropTree.put( "expert.autorangeTolerance", 0.10 );
        AppSettingsPropTree.put( "expert.smallSignalResolutionLimit", 0.0 );
        AppSettingsPropTree.put( "expert.maxAutorangeAmplitude", 1.0 );
        AppSettingsPropTree.put( "expert.minCyclesCaptured", 16 ); // Bin width 6.25% of stimulus frequency

        settingsFileOutputStream.open( appDataFilename.c_str(), ios::out );

        if (settingsFileOutputStream)
        {
            xml_writer_settings<char> settings(' ', 4);
            write_xml(settingsFileOutputStream, AppSettingsPropTree, settings);
            appSettingsOpened = true;
            appSettingsDirty = false;
        }
        else
        {
            retVal = false;
        }
    }
    catch( const ptree_error& pte )
    {
        UNREFERENCED_PARAMETER(pte);
        retVal = false;
        settingsFileOutputStream.close();
    }

    settingsFileOutputStream.close();

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::WriteApplicationSettings
//
// Purpose: Saves the application settings back the application settings file.
//
// Parameters: [out] return - Whether the function was successful
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationSettings::WriteApplicationSettings( void )
{
    bool retVal = true;
    ofstream settingsFileOutputStream;

    if (appSettingsOpened == true && appSettingsDirty == true)
    {
        try
        {
            settingsFileOutputStream.open( appDataFilename.c_str(), ios::out );

            if (settingsFileOutputStream)
            {
                xml_writer_settings<char> settings(' ', 4);
                write_xml(settingsFileOutputStream, AppSettingsPropTree, settings);
            }
            else
            {
                retVal = false;
            }
        }
        catch( const ptree_error& pte )
        {
            UNREFERENCED_PARAMETER(pte);
            retVal = false;
            settingsFileOutputStream.close();
        }

        settingsFileOutputStream.close();
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::ReadScopeSettings
//
// Purpose: Reads the scope settings from the scope settings file for the scope specified
//
// Parameters: [in] pScope - The scope to retrieve the settings for
//             [out] return - Whether the function was successful
//
// Notes: Settings are read into a Boost property tree, where they will remain for later retrieval
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationSettings::ReadScopeSettings( PicoScope* pScope )
{
    bool retVal = true;
    wstringstream settingsFileName;
    wifstream settingsFileInputStream;

    wstring model;
    wstring serialNumber;

    numChannels = pScope->GetNumChannels();

    pScope->GetModel(model);
    pScope->GetSerialNumber(serialNumber);

    for (uint16_t i = 0; i < model.length(); i++)
    {
        UINT type = PathGetCharType( model[i] );
        if ((type == GCT_INVALID) || (type & GCT_SEPARATOR) || (type & GCT_WILD))
        {
            model[i] = wchar_t('_');
        }
    }

    for (uint16_t i = 0; i < serialNumber.length(); i++)
    {
        wchar_t c = serialNumber[i];
        UINT type = PathGetCharType( c );
        if ((type == GCT_INVALID) || (type & GCT_SEPARATOR) || (type & GCT_WILD))
        {
            serialNumber[i] = wchar_t('_');
        }
    }

    settingsFileName << L"\\FRA4PicoScope\\Fra4PicoScopeSettings_" << model << L"_" << serialNumber << L".xml";

    scopeDataFilename = appDataFolder + settingsFileName.str();
    settingsFileInputStream.open( scopeDataFilename.c_str(), ios::in );

    if (!settingsFileInputStream)
    {
        if (false == InitializeScopeSettingsFile(pScope))
        {
            retVal = false;
        }
    }
    else
    {
        settingsFileInputStream.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));

        try
        {
            ScopeSettingsPropTree.clear();
            read_xml(settingsFileInputStream, ScopeSettingsPropTree, xml_parser::trim_whitespace);
            scopeSettingsOpened = true;
            scopeSettingsDirty = false;
        }
        catch( const ptree_error& pte )
        {
            UNREFERENCED_PARAMETER(pte);
            retVal = false;
            settingsFileInputStream.close();
        }
    }

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::InitializeScopeSettingsFile
//
// Purpose: Creates an initial settings file for scope settings
//
// Parameters: [out] return - Whether the function was successful
//
// Notes: Used for creating the file for the first time.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationSettings::InitializeScopeSettingsFile(PicoScope* pScope)
{
    bool retVal = true;
    wstringstream signalVppSS;
    wstringstream startFreqSS;
    wstringstream stopFreqSS;
    double midSigGenVpp;
    wofstream settingsFileOutputStream;

    ScopeSettingsPropTree.clear();
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.name", L"A" );
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.attenuation", ATTEN_1X );
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.coupling",PS_AC );
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.dcOffset", L"0.0" );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.name", L"B" );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.attenuation", ATTEN_1X );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.coupling", PS_AC );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.dcOffset", L"0.0" );

    midSigGenVpp = floor((pScope->GetMinFuncGenFreq() + pScope->GetMaxFuncGenVpp()) / 2.0);

    signalVppSS << fixed << setprecision(1) << midSigGenVpp;
    startFreqSS << fixed << setprecision(1) << (max(1.0, pScope->GetMinFuncGenFreq())); // Make frequency at least 1.0 since 0.0 (DC) makes no sense for FRA
    stopFreqSS << fixed << setprecision(1) << (pScope->GetMaxFuncGenFreq());

    ScopeSettingsPropTree.put( L"picoScope.fraParam.signalVpp", signalVppSS.str().c_str() );
    ScopeSettingsPropTree.put( L"picoScope.fraParam.startFrequency", startFreqSS.str().c_str() );
    ScopeSettingsPropTree.put( L"picoScope.fraParam.stopFrequency", stopFreqSS.str().c_str() );
    ScopeSettingsPropTree.put( L"picoScope.fraParam.stepsPerDecade", 10 );

    settingsFileOutputStream.open( scopeDataFilename.c_str(), ios::out );
    if (!settingsFileOutputStream)
    {
        retVal = false;
    }
    else
    {
        settingsFileOutputStream.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));

        try
        {
            xml_writer_settings<wchar_t> settings(wchar_t(' '), 4);
            write_xml(settingsFileOutputStream, ScopeSettingsPropTree, settings);
        }
        catch( const ptree_error& pte )
        {
            UNREFERENCED_PARAMETER(pte);
            retVal = false;
            settingsFileOutputStream.close();
        }
    }

    numChannels = pScope->GetNumChannels();

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::SetNoScopeSettings
//
// Purpose: Setup setting for the scenario where no scope is found.
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationSettings::SetNoScopeSettings( void )
{
    numChannels = 4;

    ScopeSettingsPropTree.clear();
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.name", L"A" );
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.attenuation", ATTEN_1X );
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.coupling",PS_AC );
    ScopeSettingsPropTree.put( L"picoScope.inputChannel.dcOffset", L"0.0" );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.name", L"B" );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.attenuation", ATTEN_1X );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.coupling", PS_AC );
    ScopeSettingsPropTree.put( L"picoScope.outputChannel.dcOffset", L"0.0" );

    ScopeSettingsPropTree.put( L"picoScope.fraParam.signalVpp", L"0.0" );
    ScopeSettingsPropTree.put( L"picoScope.fraParam.startFrequency", L"0.0" );
    ScopeSettingsPropTree.put( L"picoScope.fraParam.stopFrequency", L"0.0" );
    ScopeSettingsPropTree.put( L"picoScope.fraParam.stepsPerDecade", 10 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: ApplicationSettings::WriteScopeSettings
//
// Purpose: Saves the scope settings back the scope settings file.
//
// Parameters: [out] return - Whether the function was successful
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationSettings::WriteScopeSettings(void)
{
    bool retVal = true;
    wofstream settingsFileOutputStream;

    if (scopeSettingsOpened == true && scopeSettingsDirty == true)
    {
        settingsFileOutputStream.open( scopeDataFilename.c_str(), ios::out );
        if (!settingsFileOutputStream)
        {
            retVal = false;
        }
        else
        {
            settingsFileOutputStream.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));

            try
            {
                xml_writer_settings<wchar_t> settings(wchar_t(' '), 4);
                write_xml(settingsFileOutputStream, ScopeSettingsPropTree, settings);
            }
            catch( const ptree_error& pte )
            {
                UNREFERENCED_PARAMETER(pte);
                retVal = false;
                settingsFileOutputStream.close();
            }
        }
    }

    return retVal;
}