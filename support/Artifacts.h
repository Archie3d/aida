#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace Artifacts {

std::filesystem::path nativePath(const std::string& path);
std::string pathText(const std::filesystem::path& path);
std::string targetIdentity();
using FileInputs = std::vector<std::pair<std::string, std::string>>;
// Captures content, canonical target and permissions, including absent probes.
// A known digest must describe the bytes actually consumed/produced.
std::string fileFingerprint(const std::string& path, const std::string* knownDigest = nullptr);
bool inputsMatch(const FileInputs& inputs, bool allowUnknown = false);

// A unique sibling staging directory keeps external tools on the destination
// filesystem and preserves the output suffix (.s/.o/.exe).
class AtomicPath
{
public:
    explicit AtomicPath(const std::string& destination);
    ~AtomicPath();
    AtomicPath(const AtomicPath&) = delete;
    AtomicPath& operator=(const AtomicPath&) = delete;

    const std::string& temporary() const { return m_temporary; }
    bool valid() const { return !m_temporary.empty(); }
    bool commit();

private:
    std::string m_destination;
    std::string m_temporary;
    std::filesystem::path m_directory;
};

class AtomicFile
{
public:
    explicit AtomicFile(const std::string& destination, std::ios::openmode mode = std::ios::out);
    std::ofstream& stream() { return m_stream; }
    const std::string& temporary() const { return m_path.temporary(); }
    bool finish();
    bool commit();

private:
    AtomicPath m_path;
    std::ofstream m_stream;
    bool m_finished = false;
};


// Kernel-owned exclusion survives crashes without a stale PID-file protocol.
// Children inherit the live handle and may borrow it for the same directory.
class DirectoryLock
{
public:
    DirectoryLock() = default;
    ~DirectoryLock();
    DirectoryLock(const DirectoryLock&) = delete;
    DirectoryLock& operator=(const DirectoryLock&) = delete;

    // A synthetic child identity puts file-output guards inside the writable
    // output directory while sharing one guard with front-end child processes.
    static std::string directoryForOutput(const std::string& output);
    // Canonical guard path, also used to order acquisition of multiple locks.
    static std::string pathFor(const std::string& directory, std::string& error);
    bool acquire(const std::string& directory, const std::string& inherited = {});
    std::string inheritedArgument() const;
    const std::string& error() const { return m_error; }

private:
    intptr_t m_handle = -1;
    std::string m_error;
};

} // namespace Artifacts
