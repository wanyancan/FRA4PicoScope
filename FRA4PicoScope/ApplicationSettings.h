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
// Module: ApplicationSettings.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <tuple>

#include <boost/property_tree/ptree.hpp>
using namespace boost::property_tree;

#include "PicoScopeInterface.h"
#include "PicoScopeFRA.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: class ApplicationSettings
//
// Purpose: This class supports an inteface to application and scope setting data.
//
// Parameters: N/A
//
// Notes: This class uses Boost Property tree do the bulk of the work
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class ApplicationSettings
{
    public:
        ApplicationSettings( wstring _appDataFolder );
        ~ApplicationSettings(void);
        bool ReadApplicationSettings( void );
        bool WriteApplicationSettings( void );
        bool ReadScopeSettings( PicoScope* pScope );
        bool WriteScopeSettings( void );
        void SetNoScopeSettings( void );

        // Accessors/Mutators for Application Settings

        inline string GetApplicationVersion(void)
        {
            string versionString;
            try
            {
                versionString = AppSettingsPropTree.get<string>( "appVersion" );
            }
            catch ( const ptree_error& pte )
            {
                UNREFERENCED_PARAMETER(pte);
                versionString = "unknown";
            }
            return versionString;
        }

        inline void GetMostRecentScope( ScopeDriverFamily_T& family, string& sn )
        {
            sn = AppSettingsPropTree.get<string>( "mostRecentScope.SN" );
            family = (ScopeDriverFamily_T)AppSettingsPropTree.get<int>( "mostRecentScope.family" );
        }

        inline void SetMostRecentScope( ScopeDriverFamily_T family, string sn )
        {
            AppSettingsPropTree.put( "mostRecentScope.SN", sn );
            AppSettingsPropTree.put( "mostRecentScope.family", family );
            appSettingsDirty = true;
        }

        // Accessors/Mutators for Application Settings
        inline SamplingMode_T GetSamplingMode( void )
        {
            return (SamplingMode_T)AppSettingsPropTree.get<int>( "samplingMode" );
        }
        inline void SetSamplingMode( SamplingMode_T mode )
        {
            AppSettingsPropTree.put( "samplingMode", mode );
            appSettingsDirty = true;
        }

        inline tuple<bool, double, double> GetFreqScale(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<bool>( "plot.freqAxis.autoscale" ),
                                      AppSettingsPropTree.get<double>( "plot.freqAxis.min" ),
                                      AppSettingsPropTree.get<double>( "plot.freqAxis.max" ) );
            return retVal;
        }
        inline void SetFreqScale(tuple<bool, double, double> freqScale)
        {
            AppSettingsPropTree.put( "plot.freqAxis.autoscale", get<0>(freqScale) );
            AppSettingsPropTree.put( "plot.freqAxis.min", get<1>(freqScale) );
            AppSettingsPropTree.put( "plot.freqAxis.max", get<2>(freqScale) );
            appSettingsDirty = true;
        }

        inline tuple<bool, double, double> GetGainScale(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<bool>( "plot.gainAxis.autoscale" ),
                                      AppSettingsPropTree.get<double>( "plot.gainAxis.min" ),
                                      AppSettingsPropTree.get<double>( "plot.gainAxis.max" ) );
            return retVal;
        }
        inline void SetGainScale(tuple<bool, double, double> gainScale)
        {
            AppSettingsPropTree.put( "plot.gainAxis.autoscale", get<0>(gainScale) );
            AppSettingsPropTree.put( "plot.gainAxis.min", get<1>(gainScale) );
            AppSettingsPropTree.put( "plot.gainAxis.max", get<2>(gainScale) );
            appSettingsDirty = true;
        }

        inline tuple<bool, double, double> GetPhaseScale(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<bool>( "plot.phaseAxis.autoscale" ),
                                      AppSettingsPropTree.get<double>( "plot.phaseAxis.min" ),
                                      AppSettingsPropTree.get<double>( "plot.phaseAxis.max" ) );
            return retVal;
        }
        inline void SetPhaseScale(tuple<bool, double, double> phaseScale)
        {
            AppSettingsPropTree.put( "plot.phaseAxis.autoscale", get<0>(phaseScale) );
            AppSettingsPropTree.put( "plot.phaseAxis.min", get<1>(phaseScale) );
            AppSettingsPropTree.put( "plot.phaseAxis.max", get<2>(phaseScale) );
            appSettingsDirty = true;
        }

        inline tuple<double, uint8_t, bool, bool> GetFreqIntervals(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<double>( "plot.freqAxis.majorTickInterval" ),
                                      AppSettingsPropTree.get<uint8_t>( "plot.freqAxis.minorTicksPerMajorInterval" ),
                                      AppSettingsPropTree.get<bool>( "plot.freqAxis.majorGrids" ),
                                      AppSettingsPropTree.get<bool>( "plot.freqAxis.minorGrids" ));
            return retVal;
        }
        inline void SetFreqIntervals(tuple<double, uint8_t, bool, bool> freqIntervals)
        {
            AppSettingsPropTree.put( "plot.freqAxis.majorTickInterval", get<0>(freqIntervals) );
            AppSettingsPropTree.put( "plot.freqAxis.minorTicksPerMajorInterval", get<1>(freqIntervals) );
            AppSettingsPropTree.put( "plot.freqAxis.majorGrids", get<2>(freqIntervals) );
            AppSettingsPropTree.put( "plot.freqAxis.minorGrids", get<3>(freqIntervals) );
            appSettingsDirty = true;
        }

        inline tuple<double, uint8_t, bool, bool> GetGainIntervals(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<double>( "plot.gainAxis.majorTickInterval" ),
                                      AppSettingsPropTree.get<uint8_t>( "plot.gainAxis.minorTicksPerMajorInterval" ),
                                      AppSettingsPropTree.get<bool>( "plot.gainAxis.majorGrids" ),
                                      AppSettingsPropTree.get<bool>( "plot.gainAxis.minorGrids" ));
            return retVal;
        }
        inline void SetGainIntervals(tuple<double, uint8_t, bool, bool> gainIntervals)
        {
            AppSettingsPropTree.put( "plot.gainAxis.majorTickInterval", get<0>(gainIntervals) );
            AppSettingsPropTree.put( "plot.gainAxis.minorTicksPerMajorInterval", get<1>(gainIntervals) );
            AppSettingsPropTree.put( "plot.gainAxis.majorGrids", get<2>(gainIntervals) );
            AppSettingsPropTree.put( "plot.gainAxis.minorGrids", get<3>(gainIntervals) );
            appSettingsDirty = true;
        }

        inline bool GetGainMasterIntervals(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.gainAxis.masterGrids" );
        }
        inline void SetGainMasterIntervals( bool masterIntervals )
        {
            AppSettingsPropTree.put( "plot.gainAxis.masterGrids", masterIntervals );
            appSettingsDirty = true;
        }

        inline tuple<double, uint8_t, bool, bool> GetPhaseIntervals(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<double>( "plot.phaseAxis.majorTickInterval" ),
                                      AppSettingsPropTree.get<uint8_t>( "plot.phaseAxis.minorTicksPerMajorInterval" ),
                                      AppSettingsPropTree.get<bool>( "plot.phaseAxis.majorGrids" ),
                                      AppSettingsPropTree.get<bool>( "plot.phaseAxis.minorGrids" ));
            return retVal;
        }
        inline void SetPhaseIntervals(tuple<double, uint8_t, bool, bool> phaseIntervals)
        {
            AppSettingsPropTree.put( "plot.phaseAxis.majorTickInterval", get<0>(phaseIntervals) );
            AppSettingsPropTree.put( "plot.phaseAxis.minorTicksPerMajorInterval", get<1>(phaseIntervals) );
            AppSettingsPropTree.put( "plot.phaseAxis.majorGrids", get<2>(phaseIntervals) );
            AppSettingsPropTree.put( "plot.phaseAxis.minorGrids", get<3>(phaseIntervals) );
            appSettingsDirty = true;
        }

        inline bool GetPhaseMasterIntervals(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.phaseAxis.masterGrids" );
        }
        inline void SetPhaseMasterIntervals( bool masterIntervals )
        {
            AppSettingsPropTree.put( "plot.phaseAxis.masterGrids", masterIntervals );
            appSettingsDirty = true;
        }

        inline bool GetPlotGain(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotGain" );
        }
        inline void SetPlotGain( bool plotGain )
        {
            AppSettingsPropTree.put( "plot.plotGain", plotGain );
            appSettingsDirty = true;
        }

        inline bool GetPlotPhase(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotPhase" );
        }
        inline void SetPlotPhase( bool plotPhase )
        {
            AppSettingsPropTree.put( "plot.plotPhase", plotPhase );
            appSettingsDirty = true;
        }

        inline bool GetPlotGainMargin(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotGainMargin" );
        }
        inline void SetPlotGainMargin( bool plotGainMargin )
        {
            AppSettingsPropTree.put( "plot.plotGainMargin", plotGainMargin );
            appSettingsDirty = true;
        }

        inline bool GetPlotPhaseMargin(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotPhaseMargin" );
        }
        inline void SetPlotPhaseMargin( bool plotPhaseMargin )
        {
            AppSettingsPropTree.put( "plot.plotPhaseMargin", plotPhaseMargin );
            appSettingsDirty = true;
        }

        inline bool GetPlotUnwrappedPhase(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotUnwrappedPhase" );
        }
        inline void SetPlotUnwrappedPhase( bool plotUnwrappedPhase )
        {
            AppSettingsPropTree.put( "plot.plotUnwrappedPhase", plotUnwrappedPhase );
            appSettingsDirty = true;
        }

        inline double GetPurityLowerLimit( void )
        {
            return AppSettingsPropTree.get<double>( "expert.purityLowerLimit" );
        }
        inline void SetPurityLowerLimit( double purityLowerLimit )
        {
            AppSettingsPropTree.put( "expert.purityLowerLimit", purityLowerLimit );
            appSettingsDirty = true;
        }

        inline uint16_t GetExtraSettlingTimeMs( void )
        {
            return AppSettingsPropTree.get<uint16_t>( "expert.extraSettlingTimeMs" );
        }
        inline void SetExtraSettlingTimeMs( uint16_t extraSettlingTimeMs )
        {
            AppSettingsPropTree.put( "expert.extraSettlingTimeMs", extraSettlingTimeMs );
            appSettingsDirty = true;
        }

        inline uint8_t GetAutorangeTriesPerStep( void )
        {
            return AppSettingsPropTree.get<uint8_t>( "expert.autorangeTriesPerStep" );
        }
        inline void SetAutorangeTriesPerStep( uint8_t autorangeTriesPerStep )
        {
            AppSettingsPropTree.put( "expert.autorangeTriesPerStep", autorangeTriesPerStep );
            appSettingsDirty = true;
        }

        inline double GetAutorangeTolerance( void )
        {
            return AppSettingsPropTree.get<double>( "expert.autorangeTolerance" );
        }
        inline void SetAutorangeTolerance( double autorangeTolerance )
        {
            AppSettingsPropTree.put( "expert.autorangeTolerance", autorangeTolerance );
            appSettingsDirty = true;
        }

        inline double GetSmallSignalResolutionLimit( void )
        {
            return AppSettingsPropTree.get<double>( "expert.smallSignalResolutionLimit" );
        }
        inline void SetSmallSignalResolutionLimit( double smallSignalResolutionLimit )
        {
            AppSettingsPropTree.put( "expert.smallSignalResolutionLimit", smallSignalResolutionLimit );
            appSettingsDirty = true;
        }

        inline double GetMaxAutorangeAmplitude( void )
        {
            return AppSettingsPropTree.get<double>( "expert.maxAutorangeAmplitude" );
        }
        inline void SetMaxAutorangeAmplitude( double maxAutorangeAmplitude )
        {
            AppSettingsPropTree.put( "expert.maxAutorangeAmplitude", maxAutorangeAmplitude );
            appSettingsDirty = true;
        }

        inline uint16_t GetMinCyclesCaptured( void )
        {
            return AppSettingsPropTree.get<uint16_t>( "expert.minCyclesCaptured" );
        }
        inline void SetMinCyclesCaptured( double minCyclesCaptured )
        {
            AppSettingsPropTree.put( "expert.minCyclesCaptured", minCyclesCaptured );
            appSettingsDirty = true;
        }

        inline bool GetTimeDomainPlotsEnabled( void )
        {
            return AppSettingsPropTree.get<bool>( "diagnostics.timeDomainPlots" );
        }
        inline void SetTimeDomainPlotsEnabled( bool timeDomainPlotsEnabled )
        {
            AppSettingsPropTree.put( "diagnostics.timeDomainPlots", timeDomainPlotsEnabled );
            appSettingsDirty = true;
        }

        // Accessors/Mutators for Scope Settings
        inline uint8_t GetNumChannels(void)
        {
            return numChannels;
        }

        inline int GetInputChannel()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.inputChannel.name").c_str()[0] - wchar_t('A');
        }
        inline void SetInputChannel(int channel)
        {
            wchar_t chanLetter[2] = L"A";
            chanLetter[0] += channel;
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.name", wstring(chanLetter));
            scopeSettingsDirty = true;
        }

        inline int GetInputCoupling()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.inputChannel.coupling");
        }
        inline void SetInputCoupling(int coupling)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.coupling", coupling);
            scopeSettingsDirty = true;
        }

        inline int GetInputAttenuation()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.inputChannel.attenuation");
        }
        inline void SetInputAttenuation(int attenuation)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.attenuation", attenuation);
            scopeSettingsDirty = true;
        }

        inline const wstring GetInputDcOffset()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.inputChannel.dcOffset");
        }
        inline void SetInputDcOffset(const wchar_t* dcOffset)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.dcOffset", wstring(dcOffset));
            scopeSettingsDirty = true;
        }

        inline int GetOutputChannel()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.outputChannel.name").c_str()[0] - wchar_t('A');
        }
        inline void SetOutputChannel(int channel)
        {
            wchar_t chanLetter[2] = L"A";
            chanLetter[0] += channel;
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.name", wstring(chanLetter));
            scopeSettingsDirty = true;
        }

        inline int GetOutputCoupling()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.outputChannel.coupling");
        }
        inline void SetOutputCoupling(int coupling)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.coupling", coupling);
            scopeSettingsDirty = true;
        }

        inline int GetOutputAttenuation()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.outputChannel.attenuation");
        }
        inline void SetOutputAttenuation(int attenuation)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.attenuation", attenuation);
            scopeSettingsDirty = true;
        }

        inline const wstring GetOutputDcOffset()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.outputChannel.dcOffset");
        }
        inline void SetOutputDcOffset(const wchar_t* dcOffset)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.dcOffset", wstring(dcOffset));
            scopeSettingsDirty = true;
        }

        inline const wstring GetInputSignalVpp()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.signalVpp");
        }
        inline void SetInputSignalVpp(const wchar_t* inputSignalVpp)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.signalVpp", wstring(inputSignalVpp));
            scopeSettingsDirty = true;
        }

        inline const wstring GetStartFreq()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.startFrequency");
        }
        inline void SetStartFrequency(const wchar_t* startFreq)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.startFrequency", wstring(startFreq));
            scopeSettingsDirty = true;
        }

        inline const wstring GetStopFreq()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stopFrequency");
        }
        inline void SetStopFrequency(const wchar_t* stopFreq)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stopFrequency", wstring(stopFreq));
            scopeSettingsDirty = true;
        }

        inline const wstring GetStepsPerDecade()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stepsPerDecade");
        }
        inline void SetStepsPerDecade(const wchar_t* stepsPerDecade)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stepsPerDecade", wstring(stepsPerDecade));
            scopeSettingsDirty = true;
        }

        // Versions which convert the string to a number
        inline double GetInputDcOffsetAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.inputChannel.dcOffset");
        }
        inline double GetOutputDcOffsetAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.outputChannel.dcOffset");
        }
        inline double GetInputSignalVppAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.signalVpp");
        }
        inline double GetStartFreqAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.startFrequency");
        }
        inline double GetStopFreqAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.stopFrequency");
        }
        inline int GetStepsPerDecadeAsInt()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.fraParam.stepsPerDecade");
        }

    private:

        bool InitializeScopeSettingsFile( PicoScope* pScope );
        bool InitializeApplicationSettingsFile( void );
        void CheckSettingsVersionAndUpgrade( void );

        bool appSettingsOpened;
        bool appSettingsDirty;
        bool scopeSettingsOpened;
        bool scopeSettingsDirty;

        wstring appDataFolder;
        wstring appDataFilename;
        wstring scopeDataFilename;

        ptree AppSettingsPropTree;
        wptree ScopeSettingsPropTree;

        wstring scopeSN;

        uint8_t numChannels;

};

