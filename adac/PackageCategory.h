#pragma once

// Ordered by elaboration restrictions: Pure also satisfies Preelaborate.
// Explicit values preserve the category tags in unit manifests.
enum class PackageCategory
{
    Ordinary = 0,
    Preelaborate = 1,
    Pure = 2
};
