#include "headers/hooking.h"
#include "headers/decrypt.h"
#include "headers/globals.h"
#include "headers/consolecmd.h"
#include <iostream>

static SafetyHookInline TickHook;
static bool pcCaptured = false;