#include "WindowsStack.h"

#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>

namespace
{

constexpr uint64_t kPageSize = 4096;

void skipSpace(std::string_view& text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) {
        text.remove_prefix(1);
    }
}

uint64_t stackAllocation(std::string_view text)
{
    skipSpace(text);
    if (!text.starts_with("subq")) {
        return 0;
    }
    text.remove_prefix(4);
    if (text.empty() || (text.front() != ' ' && text.front() != '\t')) {
        return 0;
    }
    skipSpace(text);
    if (text.empty() || text.front() != '$') {
        return 0;
    }
    text.remove_prefix(1);
    uint64_t bytes = 0;
    auto parsed = std::from_chars(text.data(), text.data() + text.size(), bytes);
    if (parsed.ec != std::errc {} || bytes > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
        return 0;
    }
    text.remove_prefix(static_cast<size_t>(parsed.ptr - text.data()));
    skipSpace(text);
    if (text.empty() || text.front() != ',') {
        return 0;
    }
    text.remove_prefix(1);
    skipSpace(text);
    if (!text.starts_with("%rsp")) {
        return 0;
    }
    text.remove_prefix(4);
    skipSpace(text);
    return text.empty() || text.front() == '#' ? bytes : 0;
}

}

std::string probeWindowsStack(std::string_view assembly)
{
    std::string result;
    while (!assembly.empty()) {
        size_t end = assembly.find('\n');
        bool newline = end != std::string_view::npos;
        size_t count = newline ? end : assembly.size();
        std::string_view line = assembly.substr(0, count);
        uint64_t bytes = stackAllocation(line);
        if (bytes < kPageSize) {
            result.append(line);
            if (newline) {
                result += '\n';
            }
        } else {
            // No helper call or scratch register: incoming arguments, static
            // links and live values at outgoing call-stack adjustments survive.
            // A read touches the guard page without changing the stack data.
            while (bytes != 0) {
                uint64_t step = bytes >= kPageSize ? kPageSize : bytes;
                result += "\tsubq $" + std::to_string(step) + ", %rsp\n";
                result += "\ttestb $0, (%rsp)\n";
                bytes -= step;
            }
            if (!newline) {
                result.pop_back();
            }
        }
        assembly.remove_prefix(count + (newline ? 1 : 0));
    }
    return result;
}

bool prepareWindowsAssembly(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "ada: error: cannot read assembly '" << path << "'\n";
        return false;
    }
    std::string assembly { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
    if (input.bad()) {
        std::cerr << "ada: error: cannot read assembly '" << path << "'\n";
        return false;
    }
    input.close();
    std::string probed = probeWindowsStack(assembly);
    if (probed == assembly) {
        return true;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(probed.data(), static_cast<std::streamsize>(probed.size()));
    output.close();
    if (!output) {
        std::cerr << "ada: error: cannot write assembly '" << path << "'\n";
        return false;
    }
    return true;
}
