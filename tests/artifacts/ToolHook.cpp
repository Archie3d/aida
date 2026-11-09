#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#else
#   include <unistd.h>
#endif

namespace {

std::string environment(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? std::string() : value;
}

#ifdef _WIN32
std::string quote(const std::string& argument)
{
    std::string result = "\"";
    std::size_t slashes = 0;
    for (char c : argument) {
        if (c == '\\') { ++slashes; continue; }
        result.append(slashes * (c == '"' ? 2 : 1), '\\');
        slashes = 0;
        if (c == '"') { result += '\\'; }
        result += c;
    }
    result.append(slashes * 2, '\\');
    return result + '"';
}
#endif

} // namespace

int main(int argc, char** argv)
{
    std::string tool = environment("AIDA_TEST_REAL_TOOL");
    if (tool.empty()) { return 90; }
    std::string trace = environment("AIDA_TEST_TRACE");
    if (!trace.empty()) {
        std::ofstream log(trace, std::ios::app);
        log << tool;
        for (int i = 1; i < argc; ++i) { log << ' ' << argv[i]; }
        log << '\n';
    }
    std::string output;
    bool frontend = false;
    bool source = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-o" && i + 1 < argc) { output = argv[++i]; }
        if (std::string(argv[i]) == "-c") { frontend = true; }
        if (std::filesystem::path(argv[i]).filename() == "root.adb") { source = true; }
    }
    std::string selection = environment("AIDA_TEST_SELECT");
    bool selected = (selection == "frontend" && frontend && source)
        || (!output.empty() && std::filesystem::path(output).filename().string() == selection);
    if (selected) {
        // A failed native tool is permitted to leave arbitrary partial output.
        // It must only ever see a private staging filename.
        if (!output.empty()) { std::ofstream(output, std::ios::binary) << "partial tool output"; }
        if (environment("AIDA_TEST_MODE") == "fail") { return 91; }
        std::string ready = environment("AIDA_TEST_READY");
        std::string release = environment("AIDA_TEST_RELEASE");
#ifdef _WIN32
        auto process = GetCurrentProcessId();
#else
        auto process = getpid();
#endif
        { std::ofstream marker(ready); marker << process << '\n' << output << '\n'; }
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        while (!std::filesystem::exists(release)) {
            if (std::chrono::steady_clock::now() >= deadline) { return 92; }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    std::vector<std::string> command { tool };
    for (int i = 1; i < argc; ++i) { command.emplace_back(argv[i]); }
#ifdef _WIN32
    std::string text;
    for (const auto& argument : command) { if (!text.empty()) { text += ' '; } text += quote(argument); }
    std::vector<char> buffer(text.begin(), text.end());
    buffer.push_back('\0');
    STARTUPINFOA startup {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process {};
    if (!CreateProcessA(tool.c_str(), buffer.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup, &process)) { return 93; }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD status = 1;
    GetExitCodeProcess(process.hProcess, &status);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return static_cast<int>(status);
#else
    std::vector<char*> arguments;
    for (auto& argument : command) { arguments.push_back(argument.data()); }
    arguments.push_back(nullptr);
    execvp(arguments.front(), arguments.data());
    return 93;
#endif
}
