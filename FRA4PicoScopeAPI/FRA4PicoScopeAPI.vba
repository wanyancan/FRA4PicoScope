''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'
' Frequency Response Analyzer for PicoScope
'
' Copyright (c) 2016 by Aaron Hexamer
'
' This file is part of the Frequency Response Analyzer for PicoScope program.
'
' Frequency Response Analyzer for PicoScope is free software: you can 
' redistribute it and/or modify it under the terms of the GNU General Public 
' License as published by the Free Software Foundation, either version 3 of 
' the License, or (at your option) any later version.
'
' Frequency Response Analyzer for PicoScope is distributed in the hope that 
' it will be useful,but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
' GNU General Public License for more details.
'
' You should have received a copy of the GNU General Public License
' along with Frequency Response Analyzer for PicoScope.  If not, see <http:'www.gnu.org/licenses/>.
'
' Module FRA4PicoScopeAPI.vba: VBA interface for the FRA4PicoScope API DLL
'
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Option Strict

Public Enum PS_CHANNEL
    PS_CHANNEL_A
    PS_CHANNEL_B
    PS_CHANNEL_C
    PS_CHANNEL_D
    PS_CHANNEL_E
    PS_CHANNEL_F
    PS_CHANNEL_G
    PS_CHANNEL_H
    PS_CHANNEL_INVALID
End Enum

Public Enum PS_COUPLING
    PS_AC
    PS_DC
    PS_DC_1M = PS_DC
    PS_DC_50R
End Enum

Public Enum ATTEN_T
    ATTEN_1X
    ATTEN_10X
    ATTEN_20X
    ATTEN_100X
    ATTEN_200X
    ATTEN_1000X
End Enum

Public Enum SamplingMode_T
    LOW_NOISE
    HIGH_NOISE
End Enum

Public Enum FRA_STATUS_T
    FRA_STATUS_IDLE
    FRA_STATUS_IN_PROGRESS
    FRA_STATUS_COMPLETE
    FRA_STATUS_CANCELED
    FRA_STATUS_AUTORANGE_LIMIT
    FRA_STATUS_POWER_CHANGED
    FRA_STATUS_FATAL_ERROR
    FRA_STATUS_MESSAGE
End Enum

Public Enum LOG_MESSAGE_FLAGS_T
    SCOPE_ACCESS_DIAGNOSTICS = &H0001
    FRA_PROGRESS = &H0002
    STEP_TRIAL_PROGRESS = &H0004
    SIGNAL_GENERATOR_DIAGNOSTICS = &H0008
    AUTORANGE_DIAGNOSTICS = &H0010
    ADAPTIVE_STIMULUS_DIAGNOSTICS = &H0020
    SAMPLE_PROCESSING_DIAGNOSTICS = &H0040
    DFT_DIAGNOSTICS = &H0080
    SCOPE_POWER_EVENTS = &H0100
    SAVE_EXPORT_STATUS = &H0200
    FRA_WARNING = &H0400
    FRA_ERROR = &H8000
End Enum

Declare Function SetScope Lib "FRA4PicoScope.dll" (ByVal sn As String) As Byte
Declare Function GetMinFrequency Lib "FRA4PicoScope.dll" () As Double
Declare Function StartFRA Lib "FRA4PicoScope.dll" (ByVal startFreqHz As Double, ByVal stopFreqHz As Double, ByVal stepsPerDecade As Long) As Byte
Declare Function CancelFRA Lib "FRA4PicoScope.dll" () As Byte
Declare Function GetFraStatus Lib "FRA4PicoScope.dll" () As FRA_STATUS_T
Declare Sub SetFraSettings Lib "FRA4PicoScope.dll" (ByVal samplingMode As SamplingMode_T, ByVal adaptiveStimulusMode As Byte, ByVal targetResponseAmplitude As Double, _
                                                    ByVal sweepDescending As Byte, ByVal phaseWrappingThreshold As Double)
Declare Sub SetFraTuning Lib "FRA4PicoScope.dll" (ByVal purityLowerLimit As Double, ByVal extraSettlingTimeMs As Integer, ByVal autorangeTriesPerStep As Byte, _
                                                  ByVal autorangeTolerance As Double, ByVal smallSignalResolutionTolerance As Double, ByVal maxAutorangeAmplitude As Double, _
                                                  ByVal inputStartRange As Integer, ByVal outputStartRange As Integer, ByVal adaptiveStimulusTriesPerStep As Byte, _
                                                  ByVal targetResponseAmplitudeTolerance As Double, ByVal minCyclesCaptured As Integer, ByVal maxDftBw As Double, _
                                                  ByVal lowNoiseOversampling As Integer)
Declare Function SetupChannels Lib "FRA4PicoScope.dll" (ByVal inputChannel As PS_CHANNEL, ByVal inputChannelCoupling As PS_COUPLING, ByVal inputChannelAttenuation As ATTEN_T, ByVal inputDcOffset As Double, _
                                                        ByVal outputChannel As PS_CHANNEL, ByVal outputChannelCoupling As PS_COUPLING, ByVal outputChannelAttenuation As ATTEN_T, ByVal outputDcOffset As Double, _
                                                        ByVal initialStimulusVpp As Double, ByVal maxStimulusVpp as Double, ByVal stimulusDcOffset As Double) As Byte
Declare Function GetNumSteps Lib "FRA4PicoScope.dll" () As Long
Declare Sub GetResults Lib "FRA4PicoScope.dll" (ByRef freqsLogHz As Double, ByRef gainsDb As Double, ByRef phasesDeg As Double, ByRef unwrappedPhasesDeg As Double)
Declare Sub EnableDiagnostics Lib "FRA4PicoScope.dll" (ByVal baseDataPath As String)
Declare Sub DisableDiagnostics Lib "FRA4PicoScope.dll" ()
Declare Sub AutoClearMessageLog Lib "FRA4PicoScope.dll" (ByVal bAutoClear As Byte)
Declare Sub EnableMessageLog Lib "FRA4PicoScope.dll" (ByVal bEnable As Byte)
Declare Sub SetLogVerbosityFlag Lib "FRA4PicoScope.dll" (ByVal flag As LOG_MESSAGE_FLAGS_T, ByVal enable As Byte)
Declare Sub SetLogVerbosityFlags Lib "FRA4PicoScope.dll" (ByVal flags As LOG_MESSAGE_FLAGS_T)
Declare Function GetMessageLog Lib "FRA4PicoScope.dll" () As String
Declare Sub ClearMessageLog Lib "FRA4PicoScope.dll" ()
Declare Function Initialize Lib "FRA4PicoScope.dll" () As Byte
Declare Sub Cleanup Lib "FRA4PicoScope.dll" ()