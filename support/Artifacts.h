#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace Artifacts {

std::filesystem::path nativePath(const std::string& path);
std::string pathText(const std::filesystem::path& path);

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

    bool acquire(const std::string& directory, const std::string& inherited = {});
    std::string inheritedArgument() const;
    const std::string& error() const { return m_error; }

private:
    intptr_t m_handle = -1;
    std::string m_error;
};

} // namespace Artifacts
