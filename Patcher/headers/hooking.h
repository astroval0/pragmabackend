#pragma once
#include "safetyhook.hpp"
#include "headers/consolecmd.h"

extern SafetyHookInline TickHook;

void InitHooking();
__int64 __fastcall TickHookFn(APlayerController* pc, void* unused, float deltaSeconds);