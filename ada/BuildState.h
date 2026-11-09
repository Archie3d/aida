#pragma once

#include "../support/Artifacts.h"

#include <string>
#include <vector>

// A checked receipt for one complete successful driver invocation. Input
// probes include absent candidates, so no directory scan or frontend is needed
// to reject a newly shadowed lookup.
class BuildState
{
public:
    BuildState(std::string path, std::string configuration);
    bool isCurrent() const;
    bool save(const Artifacts::FileInputs& inputs, const std::vector<std::string>& products) const;

private:
    std::string m_path;
    std::string m_configuration;
};

std::string environmentDigest();
std::string resolveProgram(const std::string& program);
