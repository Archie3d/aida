#include "Artifacts.h"
#include "../common/Digest.h"

#include <atomic>
#include <bit>
#include <cerrno>
#include <cstdlib>
#include <system_error>

#ifdef _WIN32
#   ifndef _WIN32_WINNT
#       define _WIN32_WINNT 0x0600
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#else
#   include <fcntl.h>
#   include <sys/file.h>
#   include <sys/stat.h>
#   include <unistd.h>
#endif

namespace Artifacts {
namespace {

std::atomic<uint64_t> temporaryCounter { 0 };
constexpr const char* kTemporaryPrefix = ".aida-tmp-";

uint64_t processId()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<uint64_t>(getpid());
#endif
}

bool replace(const std::string& source, const std::string& destination)
{
#ifdef _WIN32
    return MoveFileExW(nativePath(source).c_str(), nativePath(destination).c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(source.c_str(), destination.c_str()) == 0;
#endif
}

} // namespace

std::filesystem::path nativePath(const std::string& path)
{
    return std::filesystem::path(std::u8string(path.begin(), path.end()));
}

std::string pathText(const std::filesystem::path& path)
{
    auto bytes = path.u8string();
    return std::string(bytes.begin(), bytes.end());
}

// A universal executable can have the same binary digest on different CPUs.
// Keep the active ABI explicit instead of relying on executable identity alone.
std::string targetIdentity()
{
    std::string target;
#if defined(_WIN32)
    target = "windows";
#elif defined(__APPLE__)
    target = "darwin";
#elif defined(__linux__)
    target = "linux";
#else
    target = "other";
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
    target += "-arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    target += "-amd64";
#else
    target += "-other";
#endif
    target += "-pointer" + std::to_string(sizeof(void*) * 8);
    target += std::endian::native == std::endian::little ? "-le" : "-be";
    return target;
}

std::string fileFingerprint(const std::string& path, const std::string* knownDigest)
{
    std::error_code error;
    auto native = nativePath(path);
    auto absolute = std::filesystem::absolute(native, error);
    if (error) { return {}; }
    auto canonical = std::filesystem::weakly_canonical(absolute, error);
    if (error == std::errc::not_a_directory || error == std::errc::no_such_file_or_directory) {
        auto prefix = absolute;
        std::vector<std::filesystem::path> suffix;
        do {
            if (prefix == prefix.parent_path()) { return {}; }
            suffix.push_back(prefix.filename());
            prefix = prefix.parent_path();
            error.clear();
            canonical = std::filesystem::weakly_canonical(prefix, error);
        } while (error == std::errc::not_a_directory || error == std::errc::no_such_file_or_directory);
        if (!error) { for (auto part = suffix.rbegin(); part != suffix.rend(); ++part) { canonical /= *part; } }
    }
    if (error) { return {}; }
#ifdef _WIN32
    // Resolve the existing prefix as well as the leaf: an absent probe under
    // a directory symlink must notice when that directory is retargeted.
    auto prefix = canonical;
    std::vector<std::filesystem::path> suffix;
    for (;;) {
        HANDLE handle = CreateFileW(prefix.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            std::wstring resolved(32768, L'\0');
            DWORD size = GetFinalPathNameByHandleW(handle, resolved.data(), static_cast<DWORD>(resolved.size()), FILE_NAME_NORMALIZED);
            CloseHandle(handle);
            if (size == 0 || size >= resolved.size()) { return {}; }
            resolved.resize(size);
            canonical = std::filesystem::path(resolved);
            for (auto part = suffix.rbegin(); part != suffix.rend(); ++part) { canonical /= *part; }
            break;
        }
        if (prefix == prefix.parent_path()) { return {}; }
        suffix.push_back(prefix.filename());
        prefix = prefix.parent_path();
    }
#endif
    auto status = std::filesystem::status(native, error);
    std::string kind;
    if (status.type() == std::filesystem::file_type::not_found || error == std::errc::no_such_file_or_directory
        || error == std::errc::not_a_directory) { kind = "missing"; }
    else if (error) { return {}; }
    else if (std::filesystem::is_regular_file(status)) { kind = "file"; }
    else if (std::filesystem::is_directory(status)) { kind = "directory"; }
    else { return {}; }
    if (knownDigest != nullptr && kind != "file") { return {}; }
    std::string state = kind;
    state.push_back('\0');
    state += pathText(canonical);
    state.push_back('\0');
    if (kind != "missing") { state += std::to_string(static_cast<unsigned>(status.permissions())); }
    if (kind == "file") {
        std::string content = knownDigest == nullptr ? digestOfFile(path) : *knownDigest;
        if (content.empty()) { return {}; }
        state.push_back('\0');
        state += content;
    }
    return digestOf(state);
}

bool inputsMatch(const FileInputs& inputs, bool allowUnknown)
{
    for (const auto& [path, fingerprint] : inputs) {
        if (fingerprint.empty() || fingerprint == "-") {
            if (allowUnknown) { continue; }
            return false;
        }
        if (fileFingerprint(path) != fingerprint) { return false; }
    }
    return true;
}

AtomicPath::AtomicPath(const std::string& destination)
    : m_destination(destination)
{
    auto target = nativePath(destination);
    auto parent = target.parent_path();
    if (parent.empty()) { parent = "."; }
    for (int attempt = 0; attempt < 100; ++attempt) {
        auto directory = parent / (std::string(kTemporaryPrefix) + std::to_string(processId())
            + "-" + std::to_string(temporaryCounter.fetch_add(1)));
        std::error_code error;
        if (std::filesystem::create_directory(directory, error)) {
            m_directory = directory;
            m_temporary = pathText(directory / target.filename());
            return;
        }
        if (error && error != std::errc::file_exists) { return; }
    }
}

AtomicPath::~AtomicPath()
{
    if (!m_directory.empty()) {
        std::error_code ignored;
        std::filesystem::remove_all(m_directory, ignored);
    }
}

bool AtomicPath::commit()
{
    return valid() && replace(m_temporary, m_destination);
}

AtomicFile::AtomicFile(const std::string& destination, std::ios::openmode mode)
    : m_path(destination)
{
    if (m_path.valid()) { m_stream.open(nativePath(m_path.temporary()), mode | std::ios::out); }
    else { m_stream.setstate(std::ios::failbit); }
}

bool AtomicFile::finish()
{
    if (!m_finished) {
        m_stream.flush();
        m_stream.close();
        m_finished = true;
    }
    return static_cast<bool>(m_stream);
}

bool AtomicFile::commit()
{
    return finish() && m_path.commit();
}

DirectoryLock::~DirectoryLock()
{
    // Do not explicitly unlock: inherited handles may still belong to a live
    // compiler/backend after its driver was terminated.
    if (m_handle != -1) {
#ifdef _WIN32
        CloseHandle(reinterpret_cast<HANDLE>(m_handle));
#else
        close(static_cast<int>(m_handle));
#endif
    }
}

std::string DirectoryLock::directoryForOutput(const std::string& output)
{
    auto parent = nativePath(output).parent_path();
    return pathText(parent / ".aida-outputs");
}

std::string DirectoryLock::pathFor(const std::string& directory, std::string& message)
{
    std::error_code error;
    auto absolute = std::filesystem::absolute(nativePath(directory), error);
    if (error) { message = error.message(); return {}; }
    auto root = std::filesystem::weakly_canonical(absolute, error);
    if (error) { message = error.message(); return {}; }
    auto parent = root.parent_path();
    std::filesystem::create_directories(parent, error);
    if (error) { message = error.message(); return {}; }
#ifdef _WIN32
    // MinGW weakly_canonical can retain a directory symlink spelling. Use
    // the opened target so aliases select the same persistent guard file.
    HANDLE directoryHandle = CreateFileW(root.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    bool missingRoot = directoryHandle == INVALID_HANDLE_VALUE;
    if (missingRoot) {
        directoryHandle = CreateFileW(parent.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    }
    if (directoryHandle == INVALID_HANDLE_VALUE) { message = "cannot resolve artifact directory"; return {}; }
    std::wstring canonical(32768, L'\0');
    DWORD canonicalSize = GetFinalPathNameByHandleW(directoryHandle, canonical.data(),
        static_cast<DWORD>(canonical.size()), FILE_NAME_NORMALIZED);
    CloseHandle(directoryHandle);
    if (canonicalSize == 0 || canonicalSize >= canonical.size()) { message = "cannot resolve artifact directory"; return {}; }
    canonical.resize(canonicalSize);
    root = missingRoot ? std::filesystem::path(canonical) / root.filename() : std::filesystem::path(canonical);
    parent = root.parent_path();
#endif
    return pathText(parent / ("." + pathText(root.filename()) + ".aida-lock"));
}

bool DirectoryLock::acquire(const std::string& directory, const std::string& inherited)
{
    std::string identity = pathFor(directory, m_error);
    if (identity.empty()) { return false; }
    auto lockPath = nativePath(identity);
    intptr_t borrowed = -1;
    if (!inherited.empty()) {
        char* end = nullptr;
        errno = 0;
        auto value = std::strtoull(inherited.c_str(), &end, 10);
        if (errno || end == inherited.c_str() || *end != '\0' || value > INTPTR_MAX) {
            m_error = "invalid inherited artifact lock";
            return false;
        }
        borrowed = static_cast<intptr_t>(value);
    }
#ifdef _WIN32
    if (borrowed != -1) {
        HANDLE handle = reinterpret_cast<HANDLE>(borrowed);
        std::wstring resolved(32768, L'\0');
        DWORD size = GetFinalPathNameByHandleW(handle, resolved.data(), static_cast<DWORD>(resolved.size()), FILE_NAME_NORMALIZED);
        if (size == 0 || size >= resolved.size()) { m_error = "invalid inherited artifact lock"; return false; }
        resolved.resize(size);
        auto normalize = [](std::wstring value) {
            if (value.starts_with(L"\\\\?\\UNC\\")) { value = L"\\\\" + value.substr(8); }
            else if (value.starts_with(L"\\\\?\\")) { value.erase(0, 4); }
            return value;
        };
        resolved = normalize(resolved);
        auto expected = normalize(lockPath.native());
        if (_wcsicmp(resolved.c_str(), expected.c_str()) != 0) {
            m_error = "inherited artifact lock belongs to a different directory";
            return false;
        }
        m_handle = borrowed;
    } else {
        SECURITY_ATTRIBUTES attributes { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        for (;;) {
            HANDLE handle = CreateFileW(lockPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                &attributes, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle != INVALID_HANDLE_VALUE) { m_handle = reinterpret_cast<intptr_t>(handle); break; }
            DWORD failure = GetLastError();
            if (failure != ERROR_SHARING_VIOLATION) { m_error = std::system_category().message(failure); return false; }
            Sleep(25);
        }
    }
#else
    if (borrowed != -1 && (borrowed > INT32_MAX || fcntl(static_cast<int>(borrowed), F_GETFD) < 0)) {
        m_error = "invalid inherited artifact lock";
        return false;
    }
    int descriptor = open(lockPath.c_str(), O_RDWR | O_CREAT, 0600);
    if (descriptor < 0) { m_error = std::generic_category().message(errno); return false; }
    if (borrowed != -1) {
        struct stat expected {};
        struct stat actual {};
        bool valid = fstat(descriptor, &expected) == 0 && fstat(static_cast<int>(borrowed), &actual) == 0
            && expected.st_dev == actual.st_dev && expected.st_ino == actual.st_ino
            && flock(static_cast<int>(borrowed), LOCK_EX | LOCK_NB) == 0;
        close(descriptor);
        if (!valid) { m_error = "invalid inherited artifact lock for this directory"; return false; }
        m_handle = borrowed;
    } else {
        while (flock(descriptor, LOCK_EX) != 0) {
            if (errno == EINTR) { continue; }
            m_error = std::generic_category().message(errno);
            close(descriptor);
            return false;
        }
        m_handle = descriptor;
    }
#endif
    return true;
}

std::string DirectoryLock::inheritedArgument() const
{
    return m_handle == -1 ? std::string() : std::to_string(m_handle);
}

} // namespace Artifacts
