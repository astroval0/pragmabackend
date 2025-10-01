#pragma once

void InitLWSHook();

// __int64 __fastcall fn(__int64 a1, int* a2)
using FnCreateVhost = __int64(__fastcall*)(__int64, int*);