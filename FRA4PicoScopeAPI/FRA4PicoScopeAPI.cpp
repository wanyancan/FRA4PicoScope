// FRA4PicoScopeAPI.cpp : Defines the implementation of the FRA4PicoScope API DLL.
//

#include "stdafx.h"
#include "FRA4PicoScopeAPI.h"
#include "ScopeSelector.h"
#include "PicoScopeFRA.h"

ScopeSelector* pScopeSelector = NULL;
PicoScopeFRA* pFRA = NULL;

void Initialize(void)
{
    pScopeSelector = new ScopeSelector();
    pFRA = new PicoScopeFRA(NULL); // TBD callback
}

void LogMessage(const wstring statusMessage)
{
    // TBD
}
