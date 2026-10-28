#pragma once

#include <cstdint>
#include <string>

namespace CheckPolicy
{

constexpr uint32_t kAccess = 1u << 0;
constexpr uint32_t kDiscriminant = 1u << 1;
constexpr uint32_t kDivision = 1u << 2;
constexpr uint32_t kIndex = 1u << 3;
constexpr uint32_t kLength = 1u << 4;
constexpr uint32_t kOverflow = 1u << 5;
constexpr uint32_t kRange = 1u << 6;
constexpr uint32_t kTag = 1u << 7;
constexpr uint32_t kAccessibility = 1u << 8;
constexpr uint32_t kAllocation = 1u << 9;
constexpr uint32_t kElaboration = 1u << 10;
constexpr uint32_t kStorage = 1u << 11;
constexpr uint32_t kAll = (1u << 12) - 1;

inline uint32_t namedCheck(const std::string& name)
{
    static const char* const kNames[] = {"access_check", "discriminant_check", "division_check",
        "index_check", "length_check", "overflow_check", "range_check", "tag_check",
        "accessibility_check", "allocation_check", "elaboration_check", "storage_check"};
    if (name == "all_checks") { return kAll; }
    for (unsigned i = 0; i < 12; ++i) {
        if (name == kNames[i]) { return 1u << i; }
    }
    return 0;
}

}

class ScopedCheckPolicy
{
public:
    ScopedCheckPolicy(uint32_t& state, uint32_t value) : m_state(state), m_saved(state) { state = value; }
    ~ScopedCheckPolicy() { m_state = m_saved; }
    ScopedCheckPolicy(const ScopedCheckPolicy&) = delete;
    ScopedCheckPolicy& operator=(const ScopedCheckPolicy&) = delete;

private:
    uint32_t& m_state;
    uint32_t m_saved;
};
