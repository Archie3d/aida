#pragma once

#include <string>
#include <string_view>

struct WindowsParameters
{
    std::string signature;
    std::string prologue;
};

// Preserve the Windows stack ABI while avoiding QBE's integer classification
// of scalar floating parameters after the four register argument slots.
WindowsParameters lowerWindowsParameters(std::string_view signature);
