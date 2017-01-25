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

// Suppress an unnecessary C4996 warning about use of checked
// iterators invoked from a ptree compare operation
#define _SCL_SECURE_NO_WARNINGS

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
        }

        // Accessors/Mutators for Application Settings
        inline SamplingMode_T GetSamplingMode( void )
        {
            return (SamplingMode_T)AppSettingsPropTree.get<int>( "samplingMode" );
        }
        inline void SetSamplingMode( SamplingMode_T mode )
        {
            AppSettingsPropTree.put( "samplingMode", mode );
        }

        inline bool GetAdaptiveStimulusMode( void )
        {
            return (bool)AppSettingsPropTree.get<bool>( "adaptiveStimulusMode" );
        }
        inline void SetSamplingMode( bool mode )
        {
            AppSettingsPropTree.put( "adaptiveStimulusMode", mode );
        }

        inline double GetTargetSignalAmplitude( void )
        {
            return (double)AppSettingsPropTree.get<double>( "targetSignalAmplitude" );
        }
        inline void SetTargetSignalAmplitude( double targetSignalAmplitude )
        {
            AppSettingsPropTree.put( "targetSignalAmplitude", targetSignalAmplitude );
        }

        inline bool GetSweepDescending( void )
        {
            return AppSettingsPropTree.get<bool>( "sweepDescending" );
        }
        inline void SetSweepDescending( bool sweepDescending )
        {
            AppSettingsPropTree.put( "sweepDescending", sweepDescending );
        }

        // Plot settings

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
        }

        inline bool GetGainMasterIntervals(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.gainAxis.masterGrids" );
        }
        inline void SetGainMasterIntervals( bool masterIntervals )
        {
            AppSettingsPropTree.put( "plot.gainAxis.masterGrids", masterIntervals );
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
        }

        inline bool GetPhaseMasterIntervals(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.phaseAxis.masterGrids" );
        }
        inline void SetPhaseMasterIntervals( bool masterIntervals )
        {
            AppSettingsPropTree.put( "plot.phaseAxis.masterGrids", masterIntervals );
        }

        inline bool GetAutoAxes(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.autoAxes" );
        }
        inline void SetAutoAxes( bool autoAxes )
        {
            AppSettingsPropTree.put( "plot.autoAxes", autoAxes );
        }

        inline bool GetPlotGain(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotGain" );
        }
        inline void SetPlotGain( bool plotGain )
        {
            AppSettingsPropTree.put( "plot.plotGain", plotGain );
        }

        inline bool GetPlotPhase(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotPhase" );
        }
        inline void SetPlotPhase( bool plotPhase )
        {
            AppSettingsPropTree.put( "plot.plotPhase", plotPhase );
        }

        inline bool GetPlotGainMargin(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotGainMargin" );
        }
        inline void SetPlotGainMargin( bool plotGainMargin )
        {
            AppSettingsPropTree.put( "plot.plotGainMargin", plotGainMargin );
        }

        inline bool GetPlotPhaseMargin(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotPhaseMargin" );
        }
        inline void SetPlotPhaseMargin( bool plotPhaseMargin )
        {
            AppSettingsPropTree.put( "plot.plotPhaseMargin", plotPhaseMargin );
        }

        inline bool GetPlotUnwrappedPhase(void)
        {
            return AppSettingsPropTree.get<bool>( "plot.plotUnwrappedPhase" );
        }
        inline void SetPlotUnwrappedPhase( bool plotUnwrappedPhase )
        {
            AppSettingsPropTree.put( "plot.plotUnwrappedPhase", plotUnwrappedPhase );
        }

        inline double GetPhaseWrappingThreshold(void)
        {
            return AppSettingsPropTree.get<double>( "plot.phaseWrappingThreshold" );
        }
        inline void SetPhaseWrappingThreshold( double phaseWrappingThreshold )
        {
            AppSettingsPropTree.put( "plot.phaseWrappingThreshold", phaseWrappingThreshold );
        }

        inline double GetGainMarginPhaseCrossover(void)
        {
            return AppSettingsPropTree.get<double>( "plot.gainMarginPhaseCrossover" );
        }
        inline void SetGainMarginPhaseCrossover( double gainMarginPhaseCrossover )
        {
            AppSettingsPropTree.put( "plot.gainMarginPhaseCrossover", gainMarginPhaseCrossover );
        }

        // Expert settings

        inline double GetPurityLowerLimit( void )
        {
            return AppSettingsPropTree.get<double>( "expert.purityLowerLimit" );
        }
        inline void SetPurityLowerLimit( double purityLowerLimit )
        {
            AppSettingsPropTree.put( "expert.purityLowerLimit", purityLowerLimit );
        }

        inline uint16_t GetExtraSettlingTimeMs( void )
        {
            return AppSettingsPropTree.get<uint16_t>( "expert.extraSettlingTimeMs" );
        }
        inline void SetExtraSettlingTimeMs( uint16_t extraSettlingTimeMs )
        {
            AppSettingsPropTree.put( "expert.extraSettlingTimeMs", extraSettlingTimeMs );
        }

        inline uint8_t GetAutorangeTriesPerStep( void )
        {
            return AppSettingsPropTree.get<uint8_t>( "expert.autorangeTriesPerStep" );
        }
        inline void SetAutorangeTriesPerStep( uint8_t autorangeTriesPerStep )
        {
            AppSettingsPropTree.put( "expert.autorangeTriesPerStep", autorangeTriesPerStep );
        }

        inline double GetAutorangeTolerance( void )
        {
            return AppSettingsPropTree.get<double>( "expert.autorangeTolerance" );
        }
        inline void SetAutorangeTolerance( double autorangeTolerance )
        {
            AppSettingsPropTree.put( "expert.autorangeTolerance", autorangeTolerance );
        }

        inline double GetSmallSignalResolutionLimit( void )
        {
            return AppSettingsPropTree.get<double>( "expert.smallSignalResolutionLimit" );
        }
        inline void SetSmallSignalResolutionLimit( double smallSignalResolutionLimit )
        {
            AppSettingsPropTree.put( "expert.smallSignalResolutionLimit", smallSignalResolutionLimit );
        }

        inline double GetMaxAutorangeAmplitude( void )
        {
            return AppSettingsPropTree.get<double>( "expert.maxAutorangeAmplitude" );
        }
        inline void SetMaxAutorangeAmplitude( double maxAutorangeAmplitude )
        {
            AppSettingsPropTree.put( "expert.maxAutorangeAmplitude", maxAutorangeAmplitude );
        }

        inline uint8_t GetAdaptiveStimulusTriesPerStep( void )
        {
            return AppSettingsPropTree.get<uint8_t>( "expert.adaptiveStimulusTriesPerStep" );
        }
        inline void SetAdaptiveStimulusTriesPerStep( uint8_t adaptiveStimulusTriesPerStep )
        {
            AppSettingsPropTree.put( "expert.adaptiveStimulusTriesPerStep", adaptiveStimulusTriesPerStep );
        }

        inline double GetTargetSignalAmplitudeTolerance( void )
        {
            return AppSettingsPropTree.get<double>( "expert.targetSignalAmplitudeTolerance" );
        }
        inline void SetTargetSignalAmplitudeTolerance( double targetSignalAmplitudeTolerance )
        {
            AppSettingsPropTree.put( "expert.targetSignalAmplitudeTolerance", targetSignalAmplitudeTolerance );
        }

        inline uint16_t GetMinCyclesCaptured( void )
        {
            return AppSettingsPropTree.get<uint16_t>( "expert.minCyclesCaptured" );
        }
        inline void SetMinCyclesCaptured( double minCyclesCaptured )
        {
            AppSettingsPropTree.put( "expert.minCyclesCaptured", minCyclesCaptured );
        }

        inline bool GetTimeDomainPlotsEnabled( void )
        {
            return AppSettingsPropTree.get<bool>( "diagnostics.timeDomainPlots" );
        }
        inline void SetTimeDomainPlotsEnabled( bool timeDomainPlotsEnabled )
        {
            AppSettingsPropTree.put( "diagnostics.timeDomainPlots", timeDomainPlotsEnabled );
        }

        // Accessors/Mutators for Scope Settings
        inline uint8_t GetNumChannels(void)
        {
            return numAvailableChannels;
        }
        inline void SetNumChannels( uint8_t numChannels )
        {
            numAvailableChannels = numChannels;
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
        }

        inline int GetInputCoupling()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.inputChannel.coupling");
        }
        inline void SetInputCoupling(int coupling)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.coupling", coupling);
        }

        inline int GetInputAttenuation()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.inputChannel.attenuation");
        }
        inline void SetInputAttenuation(int attenuation)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.attenuation", attenuation);
        }

        inline const wstring GetInputDcOffset()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.inputChannel.dcOffset");
        }
        inline void SetInputDcOffset(const wchar_t* dcOffset)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.dcOffset", wstring(dcOffset));
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
        }

        inline int GetOutputCoupling()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.outputChannel.coupling");
        }
        inline void SetOutputCoupling(int coupling)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.coupling", coupling);
        }

        inline int GetOutputAttenuation()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.outputChannel.attenuation");
        }
        inline void SetOutputAttenuation(int attenuation)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.attenuation", attenuation);
        }

        inline const wstring GetOutputDcOffset()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.outputChannel.dcOffset");
        }
        inline void SetOutputDcOffset(const wchar_t* dcOffset)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.dcOffset", wstring(dcOffset));
        }

        inline const wstring GetStimulusVpp()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stimulusVpp");
        }
        inline void SetStimulusVpp(const wchar_t* stimulusVpp)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stimulusVpp", wstring(stimulusVpp));
        }

        inline const wstring GetMaxStimulusVpp()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.maxStimulusVpp");
        }
        inline void SetMaxStimulusVpp(const wchar_t* maxStimulusVpp)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.maxStimulusVpp", wstring(maxStimulusVpp));
        }

        inline const wstring GetStartFreq()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.startFrequency");
        }
        inline void SetStartFrequency(const wchar_t* startFreq)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.startFrequency", wstring(startFreq));
        }

        inline const wstring GetStopFreq()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stopFrequency");
        }
        inline void SetStopFrequency(const wchar_t* stopFreq)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stopFrequency", wstring(stopFreq));
        }

        inline const wstring GetStepsPerDecade()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stepsPerDecade");
        }
        inline void SetStepsPerDecade(const wchar_t* stepsPerDecade)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stepsPerDecade", wstring(stepsPerDecade));
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
        inline double GetStimulusVppAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.stimulusVpp");
        }
        inline double GetMaxStimulusVppAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.maxStimulusVpp");
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

        wstring appDataFolder;
        wstring appDataFilename;
        wstring scopeDataFilename;

        ptree AppSettingsPropTree;
        ptree AppSettingsPropTreeClean;
        wptree ScopeSettingsPropTree;
        wptree ScopeSettingsPropTreeClean;

        wstring scopeSN;

        uint8_t numAvailableChannels;

};

