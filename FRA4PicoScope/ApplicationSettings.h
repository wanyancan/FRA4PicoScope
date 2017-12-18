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
#if !defined(_SCL_SECURE_NO_WARNINGS)
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <string>
#include <codecvt>
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

        inline string GetApplicationSettingsVersion(void)
        {
            string versionString;
            wstring versionStringW;
            try
            {
                versionStringW = AppSettingsPropTree.get<wstring>( L"appVersion" );
                versionString = wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(versionStringW);
            }
            catch ( const ptree_error& pte )
            {
                UNREFERENCED_PARAMETER(pte);
                versionString = "unknown";
            }

            return versionString;
        }

        inline string GetScopeSettingsVersion(void)
        {
            string versionString;
            wstring versionStringW;
            try
            {
                versionStringW = ScopeSettingsPropTree.get<wstring>( L"appVersion" );
                versionString = wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(versionStringW);
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
            wstring snW = AppSettingsPropTree.get<wstring>( L"mostRecentScope.SN" );
            sn = wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(snW);
            family = (ScopeDriverFamily_T)AppSettingsPropTree.get<int>( L"mostRecentScope.family" );
        }

        inline void SetMostRecentScope( ScopeDriverFamily_T family, string sn )
        {
            wstring snW = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(sn);
            AppSettingsPropTree.put( L"mostRecentScope.SN", snW );
            AppSettingsPropTree.put( L"mostRecentScope.family", family );
        }

        // Accessors/Mutators for Application Settings
        inline SamplingMode_T GetSamplingMode( void )
        {
            return (SamplingMode_T)AppSettingsPropTree.get<int>( L"samplingMode" );
        }
        inline void SetSamplingMode( SamplingMode_T mode )
        {
            AppSettingsPropTree.put( L"samplingMode", mode );
        }

        inline bool GetAdaptiveStimulusMode( void )
        {
            return (bool)AppSettingsPropTree.get<bool>( L"adaptiveStimulusMode" );
        }
        inline void SetAdaptiveStimulusMode( bool mode )
        {
            AppSettingsPropTree.put( L"adaptiveStimulusMode", mode );
        }

        inline const wstring GetTargetResponseAmplitudeAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"targetResponseAmplitude" );
        }
        inline double GetTargetResponseAmplitudeAsDouble( void )
        {
            return (double)AppSettingsPropTree.get<double>( L"targetResponseAmplitude" );
        }
        inline void SetTargetResponseAmplitude( wchar_t* targetResponseAmplitude )
        {
            AppSettingsPropTree.put( L"targetResponseAmplitude", targetResponseAmplitude );
        }

        inline bool GetSweepDescending( void )
        {
            return AppSettingsPropTree.get<bool>( L"sweepDescending" );
        }
        inline void SetSweepDescending( bool sweepDescending )
        {
            AppSettingsPropTree.put( L"sweepDescending", sweepDescending );
        }

        // Plot settings

        inline tuple<bool, double, double> GetFreqScale(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<bool>( L"plot.freqAxis.autoscale" ),
                                      AppSettingsPropTree.get<double>( L"plot.freqAxis.min" ),
                                      AppSettingsPropTree.get<double>( L"plot.freqAxis.max" ) );
            return retVal;
        }
        inline void SetFreqScale(tuple<bool, double, double> freqScale)
        {
            AppSettingsPropTree.put( L"plot.freqAxis.autoscale", get<0>(freqScale) );
            AppSettingsPropTree.put( L"plot.freqAxis.min", get<1>(freqScale) );
            AppSettingsPropTree.put( L"plot.freqAxis.max", get<2>(freqScale) );
        }

        inline tuple<bool, double, double> GetGainScale(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<bool>( L"plot.gainAxis.autoscale" ),
                                      AppSettingsPropTree.get<double>( L"plot.gainAxis.min" ),
                                      AppSettingsPropTree.get<double>( L"plot.gainAxis.max" ) );
            return retVal;
        }
        inline void SetGainScale(tuple<bool, double, double> gainScale)
        {
            AppSettingsPropTree.put( L"plot.gainAxis.autoscale", get<0>(gainScale) );
            AppSettingsPropTree.put( L"plot.gainAxis.min", get<1>(gainScale) );
            AppSettingsPropTree.put( L"plot.gainAxis.max", get<2>(gainScale) );
        }

        inline tuple<bool, double, double> GetPhaseScale(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<bool>( L"plot.phaseAxis.autoscale" ),
                                      AppSettingsPropTree.get<double>( L"plot.phaseAxis.min" ),
                                      AppSettingsPropTree.get<double>( L"plot.phaseAxis.max" ) );
            return retVal;
        }
        inline void SetPhaseScale(tuple<bool, double, double> phaseScale)
        {
            AppSettingsPropTree.put( L"plot.phaseAxis.autoscale", get<0>(phaseScale) );
            AppSettingsPropTree.put( L"plot.phaseAxis.min", get<1>(phaseScale) );
            AppSettingsPropTree.put( L"plot.phaseAxis.max", get<2>(phaseScale) );
        }

        inline tuple<double, uint8_t, bool, bool> GetFreqIntervals(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<double>( L"plot.freqAxis.majorTickInterval" ),
                                      AppSettingsPropTree.get<uint8_t>( L"plot.freqAxis.minorTicksPerMajorInterval" ),
                                      AppSettingsPropTree.get<bool>( L"plot.freqAxis.majorGrids" ),
                                      AppSettingsPropTree.get<bool>( L"plot.freqAxis.minorGrids" ));
            return retVal;
        }
        inline void SetFreqIntervals(tuple<double, uint8_t, bool, bool> freqIntervals)
        {
            AppSettingsPropTree.put( L"plot.freqAxis.majorTickInterval", get<0>(freqIntervals) );
            AppSettingsPropTree.put( L"plot.freqAxis.minorTicksPerMajorInterval", get<1>(freqIntervals) );
            AppSettingsPropTree.put( L"plot.freqAxis.majorGrids", get<2>(freqIntervals) );
            AppSettingsPropTree.put( L"plot.freqAxis.minorGrids", get<3>(freqIntervals) );
        }

        inline tuple<double, uint8_t, bool, bool> GetGainIntervals(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<double>( L"plot.gainAxis.majorTickInterval" ),
                                      AppSettingsPropTree.get<uint8_t>( L"plot.gainAxis.minorTicksPerMajorInterval" ),
                                      AppSettingsPropTree.get<bool>( L"plot.gainAxis.majorGrids" ),
                                      AppSettingsPropTree.get<bool>( L"plot.gainAxis.minorGrids" ));
            return retVal;
        }
        inline void SetGainIntervals(tuple<double, uint8_t, bool, bool> gainIntervals)
        {
            AppSettingsPropTree.put( L"plot.gainAxis.majorTickInterval", get<0>(gainIntervals) );
            AppSettingsPropTree.put( L"plot.gainAxis.minorTicksPerMajorInterval", get<1>(gainIntervals) );
            AppSettingsPropTree.put( L"plot.gainAxis.majorGrids", get<2>(gainIntervals) );
            AppSettingsPropTree.put( L"plot.gainAxis.minorGrids", get<3>(gainIntervals) );
        }

        inline bool GetGainMasterIntervals(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.gainAxis.masterGrids" );
        }
        inline void SetGainMasterIntervals( bool masterIntervals )
        {
            AppSettingsPropTree.put( L"plot.gainAxis.masterGrids", masterIntervals );
        }

        inline tuple<double, uint8_t, bool, bool> GetPhaseIntervals(void)
        {
            auto retVal = make_tuple( AppSettingsPropTree.get<double>( L"plot.phaseAxis.majorTickInterval" ),
                                      AppSettingsPropTree.get<uint8_t>( L"plot.phaseAxis.minorTicksPerMajorInterval" ),
                                      AppSettingsPropTree.get<bool>( L"plot.phaseAxis.majorGrids" ),
                                      AppSettingsPropTree.get<bool>( L"plot.phaseAxis.minorGrids" ));
            return retVal;
        }
        inline void SetPhaseIntervals(tuple<double, uint8_t, bool, bool> phaseIntervals)
        {
            AppSettingsPropTree.put( L"plot.phaseAxis.majorTickInterval", get<0>(phaseIntervals) );
            AppSettingsPropTree.put( L"plot.phaseAxis.minorTicksPerMajorInterval", get<1>(phaseIntervals) );
            AppSettingsPropTree.put( L"plot.phaseAxis.majorGrids", get<2>(phaseIntervals) );
            AppSettingsPropTree.put( L"plot.phaseAxis.minorGrids", get<3>(phaseIntervals) );
        }

        inline bool GetPhaseMasterIntervals(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.phaseAxis.masterGrids" );
        }
        inline void SetPhaseMasterIntervals( bool masterIntervals )
        {
            AppSettingsPropTree.put( L"plot.phaseAxis.masterGrids", masterIntervals );
        }

        inline bool GetAutoAxes(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.autoAxes" );
        }
        inline void SetAutoAxes( bool autoAxes )
        {
            AppSettingsPropTree.put( L"plot.autoAxes", autoAxes );
        }

        inline bool GetPlotGain(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.plotGain" );
        }
        inline void SetPlotGain( bool plotGain )
        {
            AppSettingsPropTree.put( L"plot.plotGain", plotGain );
        }

        inline bool GetPlotPhase(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.plotPhase" );
        }
        inline void SetPlotPhase( bool plotPhase )
        {
            AppSettingsPropTree.put( L"plot.plotPhase", plotPhase );
        }

        inline bool GetPlotGainMargin(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.plotGainMargin" );
        }
        inline void SetPlotGainMargin( bool plotGainMargin )
        {
            AppSettingsPropTree.put( L"plot.plotGainMargin", plotGainMargin );
        }

        inline bool GetPlotPhaseMargin(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.plotPhaseMargin" );
        }
        inline void SetPlotPhaseMargin( bool plotPhaseMargin )
        {
            AppSettingsPropTree.put( L"plot.plotPhaseMargin", plotPhaseMargin );
        }

        inline bool GetPlotUnwrappedPhase(void)
        {
            return AppSettingsPropTree.get<bool>( L"plot.plotUnwrappedPhase" );
        }
        inline void SetPlotUnwrappedPhase( bool plotUnwrappedPhase )
        {
            AppSettingsPropTree.put( L"plot.plotUnwrappedPhase", plotUnwrappedPhase );
        }

        inline double GetPhaseWrappingThresholdAsDouble(void)
        {
            return AppSettingsPropTree.get<double>( L"plot.phaseWrappingThreshold" );
        }
        inline const wstring GetPhaseWrappingThresholdAsString(void)
        {
            return AppSettingsPropTree.get<wstring>( L"plot.phaseWrappingThreshold" );
        }
        inline void SetPhaseWrappingThreshold( const wstring phaseWrappingThreshold )
        {
            AppSettingsPropTree.put( L"plot.phaseWrappingThreshold", phaseWrappingThreshold );
        }

        inline double GetGainMarginPhaseCrossoverAsDouble(void)
        {
            return AppSettingsPropTree.get<double>( L"plot.gainMarginPhaseCrossover" );
        }
        inline const wstring GetGainMarginPhaseCrossoverAsString(void)
        {
            return AppSettingsPropTree.get<wstring>( L"plot.gainMarginPhaseCrossover" );
        }
        inline void SetGainMarginPhaseCrossover( wstring gainMarginPhaseCrossover )
        {
            AppSettingsPropTree.put( L"plot.gainMarginPhaseCrossover", gainMarginPhaseCrossover );
        }

        inline uint16_t GetExtraSettlingTimeMsAsUint16( void )
        {
            return AppSettingsPropTree.get<uint16_t>( L"extraSettlingTimeMs" );
        }
        inline const wstring GetExtraSettlingTimeMsAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"extraSettlingTimeMs" );
        }
        inline void SetExtraSettlingTimeMs( uint16_t extraSettlingTimeMs )
        {
            AppSettingsPropTree.put( L"extraSettlingTimeMs", extraSettlingTimeMs );
        }

        inline uint8_t GetAutorangeTriesPerStepAsUint8( void )
        {
            return AppSettingsPropTree.get<uint8_t>( L"autorangeTuning.autorangeTriesPerStep" );
        }
        inline const wstring GetAutorangeTriesPerStepAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"autorangeTuning.autorangeTriesPerStep" );
        }
        inline void SetAutorangeTriesPerStep( uint8_t autorangeTriesPerStep )
        {
            AppSettingsPropTree.put( L"autorangeTuning.autorangeTriesPerStep", autorangeTriesPerStep );
        }

        inline double GetAutorangeToleranceAsFraction( void )
        {
            return (AppSettingsPropTree.get<double>( L"autorangeTuning.autorangeTolerance" )) / 100.0;
        }
        inline const wstring GetAutorangeToleranceAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"autorangeTuning.autorangeTolerance" );
        }
        inline void SetAutorangeTolerance( wstring autorangeTolerance )
        {
            AppSettingsPropTree.put( L"autorangeTuning.autorangeTolerance", autorangeTolerance );
        }

        inline double GetMaxAutorangeAmplitude( void )
        {
            return AppSettingsPropTree.get<double>( L"autorangeTuning.maxAutorangeAmplitude" );
        }
        inline void SetMaxAutorangeAmplitude( double maxAutorangeAmplitude )
        {
            AppSettingsPropTree.put( L"autorangeTuning.maxAutorangeAmplitude", maxAutorangeAmplitude );
        }

        inline uint8_t GetAdaptiveStimulusTriesPerStepAsUint8( void )
        {
            return AppSettingsPropTree.get<uint8_t>( L"adaptiveStimulusTuning.adaptiveStimulusTriesPerStep" );
        }
        inline const wstring GetAdaptiveStimulusTriesPerStepAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"adaptiveStimulusTuning.adaptiveStimulusTriesPerStep" );
        }
        inline void SetAdaptiveStimulusTriesPerStep( uint8_t adaptiveStimulusTriesPerStep )
        {
            AppSettingsPropTree.put( L"adaptiveStimulusTuning.adaptiveStimulusTriesPerStep", adaptiveStimulusTriesPerStep );
        }

        inline double GetTargetResponseAmplitudeToleranceAsFraction( void )
        {
            return (AppSettingsPropTree.get<double>( L"adaptiveStimulusTuning.targetResponseAmplitudeTolerance" )) / 100.0;
        }
        inline const wstring GetTargetResponseAmplitudeToleranceAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"adaptiveStimulusTuning.targetResponseAmplitudeTolerance" );
        }
        inline void SetTargetResponseAmplitudeTolerance( wstring targetResponseAmplitudeTolerance )
        {
            AppSettingsPropTree.put( L"adaptiveStimulusTuning.targetResponseAmplitudeTolerance", targetResponseAmplitudeTolerance );
        }

        inline uint16_t GetMinCyclesCapturedAsUint16( void )
        {
            return AppSettingsPropTree.get<uint16_t>( L"sampleParam.minCyclesCaptured" );
        }
        inline const wstring GetMinCyclesCapturedAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"sampleParam.minCyclesCaptured" );
        }
        inline void SetMinCyclesCaptured( uint16_t minCyclesCaptured )
        {
            AppSettingsPropTree.put( L"sampleParam.minCyclesCaptured", minCyclesCaptured );
        }

        inline uint16_t GetLowNoiseOversamplingAsUint16( void )
        {
            return AppSettingsPropTree.get<uint16_t>( L"sampleParam.lowNoiseOversampling" );
        }
        inline const wstring GetLowNoiseOversamplingAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"sampleParam.lowNoiseOversampling" );
        }
        inline void SetLowNoiseOversampling( uint16_t lowNoiseOversample )
        {
            AppSettingsPropTree.put( L"sampleParam.lowNoiseOversampling", lowNoiseOversample );
        }

        inline double GetNoiseRejectBandwidthAsDouble(void)
        {
            return AppSettingsPropTree.get<double>( L"sampleParam.noiseRejectBandwidth" );
        }
        inline const wstring GetNoiseRejectBandwidthAsString(void)
        {
            return AppSettingsPropTree.get<wstring>( L"sampleParam.noiseRejectBandwidth" );
        }
        inline void SetNoiseRejectBandwidth( wstring noiseRejectBandwidth )
        {
            AppSettingsPropTree.put( L"sampleParam.noiseRejectBandwidth", noiseRejectBandwidth );
        }

        ////
        inline bool GetQualityLimitsState(void)
        {
            return AppSettingsPropTree.get<bool>( L"qualityLimits.enable" );
        }
        inline void SetQualityLimitsState( bool enable )
        {
            AppSettingsPropTree.put( L"qualityLimits.enable", enable );
        }

        inline double GetAmplitudeLowerLimitAsFraction( void )
        {
            return (AppSettingsPropTree.get<double>( L"qualityLimits.amplitudeLowerLimit" )) / 100.0;
        }
        inline const wstring GetAmplitudeLowerLimitAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"qualityLimits.amplitudeLowerLimit" );
        }
        inline void SetAmplitudeLowerLimit( wstring amplitudeLowerLimit )
        {
            AppSettingsPropTree.put( L"qualityLimits.amplitudeLowerLimit", amplitudeLowerLimit );
        }

        inline double GetPurityLowerLimitAsFraction( void )
        {
            return (AppSettingsPropTree.get<double>( L"qualityLimits.purityLowerLimit" )) / 100.0;
        }
        inline const wstring GetPurityLowerLimitAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"qualityLimits.purityLowerLimit" );
        }
        inline void SetPurityLowerLimit( wstring purityLowerLimit )
        {
            AppSettingsPropTree.put( L"qualityLimits.purityLowerLimit", purityLowerLimit );
        }

        inline bool GetDcExcludedFromNoiseState(void)
        {
            return AppSettingsPropTree.get<bool>( L"qualityLimits.excludeDcFromNoise" );
        }
        inline void SetDcExcludedFromNoiseState( bool enable )
        {
            AppSettingsPropTree.put( L"qualityLimits.excludeDcFromNoise", enable );
        }

        inline bool GetLogVerbosityFlag(LOG_MESSAGE_FLAGS_T flag)
        {
            return (((LOG_MESSAGE_FLAGS_T)AppSettingsPropTree.get<int>( L"diagnostics.logVerbosityFlags" ) & flag) == flag);
        }
        inline void SetLogVerbosityFlag(LOG_MESSAGE_FLAGS_T flag, bool set)
        {
            LOG_MESSAGE_FLAGS_T currentFlags = (LOG_MESSAGE_FLAGS_T)AppSettingsPropTree.get<int>( L"diagnostics.logVerbosityFlags" );
            if (set)
            {
                currentFlags  = (LOG_MESSAGE_FLAGS_T)((int)currentFlags | (int)flag);
            }
            else
            {
                currentFlags  = (LOG_MESSAGE_FLAGS_T)((int)currentFlags & ~(int)flag);
            }
            AppSettingsPropTree.put( L"diagnostics.logVerbosityFlags", (int)currentFlags );
        }
        inline void SetLogVerbosityFlags(uint16_t flags)
        {
            AppSettingsPropTree.put( L"diagnostics.logVerbosityFlags", (int)flags );
        }

        inline bool GetTimeDomainPlotsEnabled( void )
        {
            return AppSettingsPropTree.get<bool>( L"diagnostics.timeDomainPlots" );
        }
        inline void SetTimeDomainPlotsEnabled( bool timeDomainPlotsEnabled )
        {
            AppSettingsPropTree.put( L"diagnostics.timeDomainPlots", timeDomainPlotsEnabled );
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

        inline const wstring GetInputDcOffsetAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.inputChannel.dcOffset");
        }
        inline double GetInputDcOffsetAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.inputChannel.dcOffset");
        }
        inline void SetInputDcOffset(const wchar_t* dcOffset)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.dcOffset", wstring(dcOffset));
        }

        inline int GetInputStartingRange()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.inputChannel.startingRange");
        }
        inline void SetInputStartingRange(int range)
        {
            ScopeSettingsPropTree.put(L"picoScope.inputChannel.startingRange", range);
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

        inline const wstring GetOutputDcOffsetAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.outputChannel.dcOffset");
        }
        inline double GetOutputDcOffsetAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.outputChannel.dcOffset");
        }
        inline void SetOutputDcOffset(const wchar_t* dcOffset)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.dcOffset", wstring(dcOffset));
        }

        inline int GetOutputStartingRange()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.outputChannel.startingRange");
        }
        inline void SetOutputStartingRange(int range)
        {
            ScopeSettingsPropTree.put(L"picoScope.outputChannel.startingRange", range);
        }

        inline const wstring GetStimulusVppAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stimulusVpp");
        }
        inline double GetStimulusVppAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.stimulusVpp");
        }
        inline void SetStimulusVpp(const wchar_t* stimulusVpp)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stimulusVpp", wstring(stimulusVpp));
        }

        inline const wstring GetStimulusOffsetAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stimulusOffset");
        }
        inline double GetStimulusOffsetAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.stimulusOffset");
        }
        inline void SetStimulusOffset(const wchar_t* stimulusOffset)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stimulusOffset", wstring(stimulusOffset));
        }

        inline const wstring GetMaxStimulusVppAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.maxStimulusVpp");
        }
        inline double GetMaxStimulusVppAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.maxStimulusVpp");
        }
        inline void SetMaxStimulusVpp(const wchar_t* maxStimulusVpp)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.maxStimulusVpp", wstring(maxStimulusVpp));
        }

        inline const wstring GetStartFreqAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.startFrequency");
        }
        inline double GetStartFreqAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.startFrequency");
        }
        inline void SetStartFrequency(const wchar_t* startFreq)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.startFrequency", wstring(startFreq));
        }

        inline const wstring GetStopFreqAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stopFrequency");
        }
        inline double GetStopFreqAsDouble()
        {
            return ScopeSettingsPropTree.get<double>(L"picoScope.fraParam.stopFrequency");
        }
        inline void SetStopFrequency(const wchar_t* stopFreq)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stopFrequency", wstring(stopFreq));
        }

        inline const wstring GetStepsPerDecadeAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.fraParam.stepsPerDecade");
        }
        inline int GetStepsPerDecadeAsInt()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.fraParam.stepsPerDecade");
        }
        inline void SetStepsPerDecade(const wchar_t* stepsPerDecade)
        {
            ScopeSettingsPropTree.put(L"picoScope.fraParam.stepsPerDecade", wstring(stepsPerDecade));
        }

        inline int GetNoiseRejectModeTimebaseAsInt()
        {
            return ScopeSettingsPropTree.get<int>(L"picoScope.sampleParam.noiseRejectModeTimebase");
        }
        inline const wstring GetNoiseRejectModeTimebaseAsString()
        {
            return ScopeSettingsPropTree.get<wstring>(L"picoScope.sampleParam.noiseRejectModeTimebase");
        }
        inline void SetNoiseRejectModeTimebase(int timebase)
        {
            ScopeSettingsPropTree.put(L"picoScope.sampleParam.noiseRejectModeTimebase", timebase);
        }

    private:

        typedef enum
        {
            APPLICATION_SETTINGS,
            SCOPE_SETTINGS
        } Settings_T;

        bool InitializeScopeSettingsFile( PicoScope* pScope );
        bool InitializeApplicationSettingsFile( void );
        void CheckSettingsVersionAndUpgrade( Settings_T type, PicoScope* pScope = NULL );

        wstring appDataFolder;
        wstring appDataFilename;
        wstring scopeDataFilename;

        wptree AppSettingsPropTree;
        wptree AppSettingsPropTreeClean;
        wptree ScopeSettingsPropTree;
        wptree ScopeSettingsPropTreeClean;

        wstring scopeSN;

        uint8_t numAvailableChannels;

};

