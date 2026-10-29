#pragma once

#include <string>
#include <string_view>

// QBE's Windows output reserves static stack frames without touching guard
// pages. Probe allocations a page at a time without clobbering any register.
std::string probeWindowsStack(std::string_view assembly);
bool prepareWindowsAssembly(const std::string& path);
