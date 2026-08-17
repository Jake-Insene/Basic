#pragma once
#include "Debug/Fail.hpp"


#define RMDebugInfo(...) Log::debug("[ResourceManager]: " __VA_ARGS__)
#define RMFatal(...) Fatal("[ResourceManager]: " __VA_ARGS__)