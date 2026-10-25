#pragma once

// Shared by syntax and semantic types; synchronized implementations follow in
// Phase 9, while their interface contracts use the same ancestry representation.
enum class InterfaceKind
{
    None,
    Ordinary,
    Limited,
    Synchronized,
    Task,
    Protected
};
