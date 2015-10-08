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
#include "PicoScopeInterface.h"
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <complex>

typedef enum
{
    ATTEN_1X,
    ATTEN_10X,
    ATTEN_100X,
    ATTEN_1000X
} ATTEN_T;

typedef enum
{
    OK, // Measurement is acceptable
    HIGHEST_RANGE_LIMIT_REACHED, // Overflow or amplitude too high and already on the highest range
    LOWEST_RANGE_LIMIT_REACHED, // Amplitude too low for good measurement precision and already on lowest range
    CHANNEL_OVERFLOW, // Overflow flag set
    AMPLITUDE_TOO_HIGH, // Amplitude is close enough to full scale that it would be best to increase the range
    AMPLITUDE_TOO_LOW // Amplitude is low enough that range can be decreased to increase the measurement precision.
} AUTORANGE_STATUS_T;

typedef enum
{
    FRA_STATUS_PROGRESS,
    FRA_STATUS_LOG,
    FRA_STATUS_COMPLETE,
    FRA_STATUS_CANCELED,
    FRA_STATUS_AUTORANGE_LIMIT,
    FRA_STATUS_FATAL_ERROR,
    FRA_STATUS_MESSAGE
} FRA_STATUS_T;

typedef struct
{
    FRA_STATUS_T status;
    union
    {
        struct
        {
            int stepsComplete;
            int numSteps;
        } progress;
        struct
        {
            int stepsComplete;
            int numSteps;
        } cancelPoint;
        struct
        {
            AUTORANGE_STATUS_T inputChannelStatus;
            AUTORANGE_STATUS_T outputChannelStatus;
        } autorangeLimit;

    } statusData;

    wstring statusText; // used to encode log or failure messages

    struct
    {
        // Whether to proceed, or cancel the FRA execution
        // Should only be used as a response to FRA_AUTORANGE_LIMIT
        // so that we don't create a race condition.  Cancel from the
        // the UI that's asynchronous (not tied to a specific FRA
        // operation like autorange) is handled separately.
        bool proceed;

        union
        {
            struct
            {
                ATTEN_T inputAtten;
                ATTEN_T outputAtten;
                double inputAmplify;
                double outputAmplify;
            } autorangeAdjustment;
        } response;

    } responseData;

} FRA_STATUS_MESSAGE_T;

typedef enum
{
    LOW_NOISE,
    HIGH_NOISE
} SamplingMode_T;

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

        typedef bool (*FRA_STATUS_CALLBACK)(FRA_STATUS_MESSAGE_T& pFraStatus);

        PicoScopeFRA(FRA_STATUS_CALLBACK);
        ~PicoScopeFRA(void);
        void SetInstrument( PicoScope* _ps );
        double GetMinFrequency(void);
        bool ExecuteFRA( double startFreqHz, double stopFreqHz, int stepsPerDecade );
        bool CancelFRA();
        void SetFraSettings( SamplingMode_T samplingMode, double purityLowerLimit, uint16_t extraSettlingTimeMs, uint8_t autorangeTriesPerStep,
                             double autorangeTolerance, double smallSignalResolutionTolerance, double maxAutorangeAmplitude, uint16_t minCyclesCaptured,
                             double phaseWrappingThreshold, bool diagnosticsOn, wstring baseDataPath );
        bool SetupChannels( int inputChannel, int inputChannelCoupling, int inputChannelAttenuation, double inputDcOffset,
                            int outputChannel, int outputChannelCoupling, int outputChannelAttenuation, double outputDcOffset,
                            double signalVpp );
        void GetResults( int* numSteps, double** freqsLogHz, double** gainsDb, double** phasesDeg, double** unwrappedPhasesDeg );

    private:
        // Data about the scope
        PicoScope* ps;
        uint8_t numChannels;
        uint32_t maxScopeSamplesPerChannel;

        FRA_STATUS_CALLBACK StatusCallback;

        double currentFreqHz;
        double currentOutputVolts;

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

        AUTORANGE_STATUS_T inputChannelAutorangeStatus;
        AUTORANGE_STATUS_T outputChannelAutorangeStatus;

        double mPurityLowerLimit;           // Lowest allowed purity before we turn up the stimulus amplitude
        double minAllowedAmplitudeRatio;    // Lowest amplitude we will tolerate for measurement on lowest range.
        double minAmplitudeRatioTolerance;  // Tolerance so that when we step up we're not over maxAmplitudeRatio
        double maxAmplitudeRatio;           // Max we want an amplitude to be before switching ranges
        int maxAutorangeRetries;            // max number of tries to auto-range before failing
        uint16_t mExtraSettlingTimeMs;      // Extra settling time between auto-range tries
        uint16_t mMinCyclesCaptured;        // Minimum whole stimulus signal cycles to capture
        double mPhaseWrappingThreshold;     // Phase value to use as wrapping point (in degrees); absolute value should be less than 360

        // Some parameters for noise reject mode
        double fSampNoiseRejectMode;
        int timebaseNoiseRejectMode;

        double rangeCounts; // Maximum ADC value
        double signalGeneratorPrecision;

        // This function allocates data used in the FRA, which is a combination
        // of diagnostic data and sample data.
        void AllocateFraData(void);
        // These variables are for keeping diagnostic data and sample data.
        int autorangeRetryCounter;
        int freqStepCounter;
        vector<int16_t>* pInputBuffer;
        vector<int16_t>* pOutputBuffer;
        vector<vector<double>> inAmps;
        vector<vector<double>> outAmps;
        vector<vector<bool>> inOV;
        vector<vector<bool>> outOV;
        vector<vector<PS_RANGE>> inRange;
        vector<vector<PS_RANGE>> outRange;
        vector<vector<vector<int16_t>>> inputMinData;
        vector<vector<vector<int16_t>>> outputMinData;
        vector<vector<vector<int16_t>>> inputMaxData;
        vector<vector<vector<int16_t>>> outputMaxData;
        vector<vector<uint16_t>> inputAbsMax;
        vector<vector<uint16_t>> outputAbsMax;
        vector<int> diagNumSamples;
        vector<int> autoRangeTries;
        vector<double> sampleInterval;
        vector<vector<double>> inputPurity;
        vector<vector<double>> outputPurity;

        bool mDiagnosticsOn;
        wstring mBaseDataPath;
        void GenerateDiagnosticOutput(void);
        static int HandlePLplotError(const char* error);

        // Treated as an array where indices here correspond to range enums/indices
        const RANGE_INFO_T* rangeInfo;
        PS_RANGE minRange;
        PS_RANGE maxRange;

        static const double attenInfo[];

        HANDLE hCaptureEvent;
        bool cancel;

        class FraFault : public exception {};

        bool GetNumChannels(void);
        bool StartCapture( double measFreqHz );
        void GenerateFrequencyPoints();
        bool ProcessData();
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
        inline bool UpdateStatus( FRA_STATUS_MESSAGE_T &msg, FRA_STATUS_T status, int stepsComplete, int numSteps )
        {
            msg.status = status;
            if (status == FRA_STATUS_PROGRESS)
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
        inline bool UpdateStatus( FRA_STATUS_MESSAGE_T &msg, FRA_STATUS_T status, AUTORANGE_STATUS_T inputChannelStatus, AUTORANGE_STATUS_T outputChannelStatus )
        {
            msg.status = status;
            msg.statusData.autorangeLimit.inputChannelStatus = inputChannelStatus;
            msg.statusData.autorangeLimit.outputChannelStatus = outputChannelStatus;
            return StatusCallback( msg );
        }
        inline bool UpdateStatus( FRA_STATUS_MESSAGE_T &msg, FRA_STATUS_T status, const wchar_t* statusMessage )
        {
            msg.status = status;
            msg.statusData.progress.numSteps = numSteps;
            msg.statusData.progress.stepsComplete = freqStepCounter;
            msg.statusText = statusMessage;
            return StatusCallback( msg );
        }
};