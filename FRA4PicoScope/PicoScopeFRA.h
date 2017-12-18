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
// Module: PicoScopeFRA.h
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "FRA4PicoScopeInterfaceTypes.h"
#include "PicoScopeInterface.h"
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <complex>

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name: class PicoScopeFRA
//
// Purpose: This is the class supporting Frequency Response Analysis execution
//
// Parameters: N/A
//
// Notes: 
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class PicoScopeFRA
{
    public:

        PicoScopeFRA(FRA_STATUS_CALLBACK);
        ~PicoScopeFRA(void);
        void SetInstrument( PicoScope* _ps );
        double GetMinFrequency(void);
        bool ExecuteFRA( double startFreqHz, double stopFreqHz, int stepsPerDecade );
        bool CancelFRA();
        static void SetCaptureStatus(PICO_STATUS status);
        void SetFraSettings( SamplingMode_T samplingMode, bool adaptiveStimulusMode, double targetSignalAmplitude,
                             bool sweepDescending, double phaseWrappingThreshold );
        void SetFraTuning( double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                           double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude,
                           int32_t inputStartRange, int32_t outputStartRange, uint8_t adaptiveStimulusTriesPerStep,
                           double targetSignalAmplitudeTolerance, uint16_t minCyclesCaptured, double maxDftBw,
                           uint16_t lowNoiseOversampling );
        bool SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                            int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                            double initialSignalVpp, double maxSignalVpp, double stimulusDcOffset );
        void GetResults( int* numSteps, double** freqsLogHz, double** gainsDb, double** phasesDeg, double** unwrappedPhasesDeg );
        void EnableDiagnostics( wstring baseDataPath );
        void DisableDiagnostics( void );

    private:
        // Data about the scope
        PicoScope* ps;
        uint8_t numAvailableChannels;
        uint32_t maxScopeSamplesPerChannel;

        FRA_STATUS_CALLBACK StatusCallback;

        double currentFreqHz;
        double currentStimulusVpp;
        double stepStimulusVpp;

        double mStartFreqHz;
        double mStopFreqHz;
        int mStepsPerDecade;
        SamplingMode_T mSamplingMode;
        PS_CHANNEL mInputChannel;
        PS_CHANNEL mOutputChannel;
        PS_COUPLING mInputChannelCoupling;
        PS_COUPLING mOutputChannelCoupling;
        ATTEN_T mInputChannelAttenuation;
        ATTEN_T mOutputChannelAttenuation;
        PS_RANGE currentInputChannelRange;
        PS_RANGE currentOutputChannelRange;
        PS_RANGE adaptiveStimulusInputChannelRange;
        PS_RANGE adaptiveStimulusOutputChannelRange;
        double mInputDcOffset;
        double mOutputDcOffset;
        int numSteps;
        vector<double> freqsHz;
        vector<double> freqsLogHz;
        vector<double> phasesDeg;
        vector<double> unwrappedPhasesDeg;
        vector<double> gainsDb;
        int latestCompletedNumSteps;
        vector<double> latestCompletedFreqsLogHz;
        vector<double> latestCompletedPhasesDeg;
        vector<double> latestCompletedUnwrappedPhasesDeg;
        vector<double> latestCompletedGainsDb;
        double actualSampFreqHz; // Scope sampling frequency
        uint32_t numSamples;
        int32_t timeIndisposedMs;
        double currentInputMagnitude;
        double currentOutputMagnitude;
        double currentInputPhase;
        double currentOutputPhase;
        double currentInputPurity;
        double currentOutputPurity;
        bool ovIn;
        bool ovOut;
        bool delayForAcCoupling;
        double currentOutputAmplitudeVolts;
        double currentInputAmplitudeVolts;
        vector<double> idealStimulusVpp; // Recorded and used for predicting next stimulus Vpp

        AUTORANGE_STATUS_T inputChannelAutorangeStatus;
        AUTORANGE_STATUS_T outputChannelAutorangeStatus;

        double mPurityLowerLimit;           // Lowest allowed purity before we warn the user and allow action
        double minAllowedAmplitudeRatio;    // Lowest amplitude we will tolerate for measurement on lowest range.
        double minAmplitudeRatioTolerance;  // Tolerance so that when we step up we're not over maxAmplitudeRatio
        double maxAmplitudeRatio;           // Max we want an amplitude to be before switching ranges
        int maxAutorangeRetries;            // max number of tries to auto-range before failing
        uint16_t mExtraSettlingTimeMs;      // Extra settling time between auto-range tries
        uint16_t mMinCyclesCaptured;        // Minimum whole stimulus signal cycles to capture
        double mMaxDftBw;                   // Maximum bandwidth for DFT in noise reject mode
        uint16_t mLowNoiseOversampling;     // Amount to oversample the stimulus frequency in low noise mode
        bool mSweepDescending;              // Whether to sweep frequency from high to low
        bool mAdaptiveStimulus;             // Whether to adjust stimulus Vpp to target an output goal
        double mTargetResponseAmplitude;    // Target amplitude for measured signals, goal is that both signals be at least this large
        double mTargetResponseAmplitudeTolerance; // Amount the smallest signal is allowed to exceed target signal amplitude (percent)
        int32_t mInputStartRange;           // Range to start input channel; -1 means base on stimulus
        int32_t mOutputStartRange;          // Range to start output channel; -1 means base on stimulus
        double mMaxStimulusVpp;             // Maximum allowed stimulus voltage in adaptive stimulus mode
        int maxAdaptiveStimulusRetries;     // Maximum number of tries to adapt stimulus before failing
        double mPhaseWrappingThreshold;     // Phase value to use as wrapping point (in degrees); absolute value should be less than 360

        double rangeCounts; // Maximum ADC value
        double signalGeneratorPrecision;

        // This function allocates data used in the FRA, which is a combination
        // of diagnostic data and sample data.
        void AllocateFraData(void);
        // These variables are for keeping diagnostic data and sample data.
        int autorangeRetryCounter;
        int adaptiveStimulusRetryCounter;
        bool stimulusChanged;
        int freqStepCounter;
        int freqStepIndex;
        vector<int16_t>* pInputBuffer;
        vector<int16_t>* pOutputBuffer;
        vector<vector<double>> inAmps;
        vector<vector<double>> outAmps;
        vector<vector<bool>> inOV;
        vector<vector<bool>> outOV;
        vector<vector<PS_RANGE>> inRange;
        vector<vector<PS_RANGE>> outRange;
        vector<vector<double>> stimVpp;
        vector<vector<vector<int16_t>>> inputMinData;
        vector<vector<vector<int16_t>>> outputMinData;
        vector<vector<vector<int16_t>>> inputMaxData;
        vector<vector<vector<int16_t>>> outputMaxData;
        vector<vector<uint16_t>> inputAbsMax;
        vector<vector<uint16_t>> outputAbsMax;
        vector<int> diagNumSamplesToPlot;
        vector<uint32_t> diagNumStimulusCyclesCaptured;
        vector<uint32_t> diagNumSamplesCaptured;
        vector<int> autoRangeTries;
        vector<int> adaptiveStimulusTries;
        vector<int> totalRetryCounter;
        vector<double> sampleInterval;
        vector<vector<double>> inputPurity;
        vector<vector<double>> outputPurity;

        bool mDiagnosticsOn;
        wstring mBaseDataPath;
        void GenerateDiagnosticOutput(void);
        static int HandlePLplotError(const char* error);

        // Treated as an array where indices here correspond to range enums/indices
        const RANGE_INFO_T* rangeInfo;
        PS_RANGE inputMinRange;
        PS_RANGE inputMaxRange;
        PS_RANGE outputMinRange;
        PS_RANGE outputMaxRange;

        static const double attenInfo[];
        static const double stimulusBasedInitialRangeEstimateMargin;
        static const uint32_t timeDomainDiagnosticDataLengthLimit;

        HANDLE hCaptureEvent;
        static PICO_STATUS captureStatus;
        bool cancel;

        class FraFault : public exception {};

        bool StartCapture( double measFreqHz );
        void GenerateFrequencyPoints();
        bool ProcessData();
        void CalculateStepInitialStimulusVpp(void);
        bool CheckStimulusTarget(bool forceAdjust = false);
        bool CheckSignalRanges(void);
        bool CheckSignalOverflows(void);
        bool CalculateGainAndPhase( double* gain, double* phase );
        void UnwrapPhases(void);
        void InitGoertzel( uint32_t N, double fSamp, double fDetect );
        void FeedGoertzel( int16_t* inputSamples, int16_t* outputSamples, uint32_t n );
        void GetGoertzelResults( double& inputMagnitude, double& inputPhase, double& inputAmplitude, double& inputPurity,
                                 double& outputMagnitude, double& outputPhase, double& outputAmplitude, double& outputPurity );
        void TransferLatestResults(void);

        // Utilities for sending a message via the callback
        // Power State
        inline bool UpdateStatus(FRA_STATUS_MESSAGE_T &msg, FRA_STATUS_T status, bool powerState)
        {
            msg.status = status;
            msg.statusData.powerState = powerState;
            // If the user selected channels C or D and the power mode is PICO_POWER_SUPPLY_NOT_CONNECTED (a given),
            // the FRA cannot be continued.
            if (mInputChannel >= PS_CHANNEL_C || mOutputChannel >= PS_CHANNEL_C)
            {
                msg.responseData.proceed = false;
            }
            else
            {
                msg.responseData.proceed = true;
            }
            return StatusCallback(msg);
        }
        // Progress Status
        inline bool UpdateStatus( FRA_STATUS_MESSAGE_T &msg, FRA_STATUS_T status, int stepsComplete, int numSteps )
        {
            msg.status = status;
            if (status == FRA_STATUS_IN_PROGRESS ||
                status == FRA_STATUS_COMPLETE)
            {
                msg.statusData.progress.numSteps = numSteps;
                msg.statusData.progress.stepsComplete = stepsComplete;
            }
            else if ( status == FRA_STATUS_CANCELED)
            {
                msg.statusData.cancelPoint.numSteps = numSteps;
                msg.statusData.cancelPoint.stepsComplete = stepsComplete;
            }
            return StatusCallback( msg );
        }
        // Retry Limit Reached
        inline bool UpdateStatus( FRA_STATUS_MESSAGE_T &msg, FRA_STATUS_T status, AUTORANGE_STATUS_T inputChannelStatus, AUTORANGE_STATUS_T outputChannelStatus )
        {
            msg.status = status;

            msg.statusData.retryLimit.autorangeLimit.allowedTries = maxAutorangeRetries;
            msg.statusData.retryLimit.autorangeLimit.triesAttempted = autoRangeTries[freqStepIndex];
            // Use the ranges recorded at the beginning of the attempt because they may have been recomputed
            msg.statusData.retryLimit.autorangeLimit.inputRange = adaptiveStimulusInputChannelRange;
            msg.statusData.retryLimit.autorangeLimit.outputRange = adaptiveStimulusOutputChannelRange;
            msg.statusData.retryLimit.autorangeLimit.inputChannelStatus = inputChannelStatus;
            msg.statusData.retryLimit.autorangeLimit.outputChannelStatus = outputChannelStatus;
            msg.statusData.retryLimit.autorangeLimit.pRangeInfo = ps->GetRangeCaps();
            msg.statusData.retryLimit.adaptiveStimulusLimit.allowedTries = maxAdaptiveStimulusRetries;
            msg.statusData.retryLimit.adaptiveStimulusLimit.triesAttempted = adaptiveStimulusTries[freqStepIndex];
            // Use the stimulus recorded at the beginning of the attempt because it may have been recomputed
            msg.statusData.retryLimit.adaptiveStimulusLimit.stimulusVpp = stepStimulusVpp;
            msg.statusData.retryLimit.adaptiveStimulusLimit.inputResponseAmplitudeV = currentInputAmplitudeVolts;
            msg.statusData.retryLimit.adaptiveStimulusLimit.outputResponseAmplitudeV = currentOutputAmplitudeVolts;
            return StatusCallback( msg );
        }
        // Status Messages
        inline bool UpdateStatus( FRA_STATUS_MESSAGE_T &msg, FRA_STATUS_T status, const wchar_t* statusMessage, LOG_MESSAGE_FLAGS_T type = FRA_ERROR )
        {
            msg.status = status;
            msg.statusData.progress.numSteps = numSteps;
            msg.statusData.progress.stepsComplete = freqStepCounter;
            msg.statusText = statusMessage;
            msg.messageType = type;
            return StatusCallback( msg );
        }
};
