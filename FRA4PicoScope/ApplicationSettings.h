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

        inline string GetApplicationVersion(void)
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
        inline void SetSamplingMode( bool mode )
        {
            AppSettingsPropTree.put( L"adaptiveStimulusMode", mode );
        }

        inline const wstring GetTargetResponseAmplitude( void )
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

        inline double GetPhaseWrappingThreshold(void)
        {
            return AppSettingsPropTree.get<double>( L"plot.phaseWrappingThreshold" );
        }
        inline const wstring GetPhaseWrappingThresholdAsString(void)
        {
            return AppSettingsPropTree.get<wstring>( L"plot.phaseWrappingThreshold" );
        }
        inline void SetPhaseWrappingThreshold( double phaseWrappingThreshold )
        {
            AppSettingsPropTree.put( L"plot.phaseWrappingThreshold", phaseWrappingThreshold );
        }

        inline double GetGainMarginPhaseCrossover(void)
        {
            return AppSettingsPropTree.get<double>( L"plot.gainMarginPhaseCrossover" );
        }
        inline const wstring GetGainMarginPhaseCrossoverAsString(void)
        {
            return AppSettingsPropTree.get<wstring>( L"plot.gainMarginPhaseCrossover" );
        }
        inline void SetGainMarginPhaseCrossover( double gainMarginPhaseCrossover )
        {
            AppSettingsPropTree.put( L"plot.gainMarginPhaseCrossover", gainMarginPhaseCrossover );
        }

        // Expert settings

        inline uint16_t GetExtraSettlingTimeMs( void )
        {
            return AppSettingsPropTree.get<uint16_t>( L"expert.extraSettlingTimeMs" );
        }
        inline const wstring GetExtraSettlingTimeAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.extraSettlingTimeMs" );
        }
        inline void SetExtraSettlingTimeMs( uint16_t extraSettlingTimeMs )
        {
            AppSettingsPropTree.put( L"expert.extraSettlingTimeMs", extraSettlingTimeMs );
        }

        inline uint8_t GetAutorangeTriesPerStep( void )
        {
            return AppSettingsPropTree.get<uint8_t>( L"expert.autorangeTriesPerStep" );
        }
        inline const wstring GetAutorangeTriesPerStepAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.autorangeTriesPerStep" );
        }
        inline void SetAutorangeTriesPerStep( uint8_t autorangeTriesPerStep )
        {
            AppSettingsPropTree.put( L"expert.autorangeTriesPerStep", autorangeTriesPerStep );
        }

        inline double GetAutorangeTolerance( void )
        {
            return (AppSettingsPropTree.get<double>( L"expert.autorangeTolerance" )) / 100.0;
        }
        inline const wstring GetAutorangeToleranceAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.autorangeTolerance" );
        }
        inline void SetAutorangeTolerance( double autorangeTolerance )
        {
            AppSettingsPropTree.put( L"expert.autorangeTolerance", autorangeTolerance );
        }

        inline double GetMaxAutorangeAmplitude( void )
        {
            return AppSettingsPropTree.get<double>( L"expert.maxAutorangeAmplitude" );
        }
        inline void SetMaxAutorangeAmplitude( double maxAutorangeAmplitude )
        {
            AppSettingsPropTree.put( L"expert.maxAutorangeAmplitude", maxAutorangeAmplitude );
        }

        inline uint8_t GetAdaptiveStimulusTriesPerStep( void )
        {
            return AppSettingsPropTree.get<uint8_t>( L"expert.adaptiveStimulusTriesPerStep" );
        }
        inline const wstring GetAdaptiveStimulusTriesPerStepAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.adaptiveStimulusTriesPerStep" );
        }
        inline void SetAdaptiveStimulusTriesPerStep( uint8_t adaptiveStimulusTriesPerStep )
        {
            AppSettingsPropTree.put( L"expert.adaptiveStimulusTriesPerStep", adaptiveStimulusTriesPerStep );
        }

        inline double GetTargetResponseAmplitudeTolerance( void )
        {
            return (AppSettingsPropTree.get<double>( L"expert.targetResponseAmplitudeTolerance" )) / 100.0;
        }
        inline const wstring GetTargetResponseAmplitudeToleranceAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.targetResponseAmplitudeTolerance" );
        }
        inline void SetTargetResponseAmplitudeTolerance( double targetResponseAmplitudeTolerance )
        {
            AppSettingsPropTree.put( L"expert.targetResponseAmplitudeTolerance", targetResponseAmplitudeTolerance );
        }

        inline uint16_t GetMinCyclesCaptured( void )
        {
            return AppSettingsPropTree.get<uint16_t>( L"expert.minCyclesCaptured" );
        }
        inline const wstring GetMinCyclesCapturedAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.minCyclesCaptured" );
        }
        inline void SetMinCyclesCaptured( double minCyclesCaptured )
        {
            AppSettingsPropTree.put( L"expert.minCyclesCaptured", minCyclesCaptured );
        }

        inline uint16_t GetMaxCyclesCaptured( void )
        {
            return AppSettingsPropTree.get<uint16_t>( L"expert.maxCyclesCaptured" );
        }
        inline const wstring GetMaxCyclesCapturedAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.maxCyclesCaptured" );
        }
        inline void SetMaxCyclesCaptured( double maxCyclesCaptured )
        {
            AppSettingsPropTree.put( L"expert.maxCyclesCaptured", maxCyclesCaptured );
        }

        inline uint16_t GetLowNoiseOversamplingAsNumber( void )
        {
            return AppSettingsPropTree.get<uint16_t>( L"expert.lowNoiseOversampling" );
        }
        inline const wstring GetLowNoiseOversamplingAsString( void )
        {
            return AppSettingsPropTree.get<wstring>( L"expert.lowNoiseOversampling" );
        }
        inline void SetLowNoiseOversampling( double minCyclesCaptured )
        {
            AppSettingsPropTree.put( L"expert.lowNoiseOversampling", minCyclesCaptured );
        }

        inline double GetNoiseRejectBandwidth(void)
        {
            return AppSettingsPropTree.get<double>( L"expert.noiseRejectBandwidth" );
        }
        inline const wstring GetNoiseRejectBandwidthAsString(void)
        {
            return AppSettingsPropTree.get<wstring>( L"expert.noiseRejectBandwidth" );
        }
        inline void SetNoiseRejectBandwidth( double noiseRejectBandwidth )
        {
            AppSettingsPropTree.put( L"expert.noiseRejectBandwidth", noiseRejectBandwidth );
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
        inline void SetAmplitudeLowerLimit( double amplitudeLowerLimit )
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
        inline void SetPurityLowerLimit( double purityLowerLimit )
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

        wptree AppSettingsPropTree;
        wptree AppSettingsPropTreeClean;
        wptree ScopeSettingsPropTree;
        wptree ScopeSettingsPropTreeClean;

        wstring scopeSN;

        uint8_t numAvailableChannels;

};

