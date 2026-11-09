#pragma once

#include <string>
#include <string_view>

// QBE entry-signature adaptation for the Windows x64 calling convention.
// Stack-passed floating arguments arrive in integer carriers; the prologue
// restores the original floating operand names before the function body.
struct WindowsParameters
{
    std::string signature; // Rewritten QBE function signature with carrier parameters.
    std::string prologue;  // QBE instructions that recover the original float/double operands.
};

// Preserve the Windows stack ABI while avoiding QBE's integer classification
// of scalar floating parameters after the four register argument slots.
WindowsParameters lowerWindowsParameters(std::string_view signature);
