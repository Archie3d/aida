#include "Diagnostics.h"
#include "DiagnosticWidth.h"
#include "Unicode.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <utility>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/ioctl.h>
#   include <unistd.h>
#endif

namespace {

bool terminal()
{
#ifdef _WIN32
    DWORD mode;
    return GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &mode) != 0;
#else
    return isatty(STDERR_FILENO) != 0;
#endif
}

int terminalWidth()
{
    constexpr int kDefaultWidth = 100;
    int width = kDefaultWidth;
    if (const char* columns = std::getenv("COLUMNS")) {
        std::string value(columns);
        int parsed = 0;
        auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (result.ec == std::errc() && result.ptr == value.data() + value.size()) { width = parsed; }
    } else {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &info)) {
            width = info.srWindow.Right - info.srWindow.Left + 1;
        }
#else
        winsize size {};
        if (ioctl(STDERR_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col != 0) { width = size.ws_col; }
#endif
    }
    return std::clamp(width, 40, 240);
}

// Escape source controls, including bidi formatting characters, so source text
// cannot inject terminal commands or reorder the diagnostic's visual labels.
std::string escaped(uint32_t code)
{
    constexpr char kHex[] = "0123456789ABCDEF";
    std::string digits;
    do {
        digits.insert(digits.begin(), kHex[code & 15]);
        code >>= 4;
    } while (code != 0);
    return "\\u{" + digits + "}";
}

struct DisplayLine
{
    std::vector<std::string> cells;
    std::vector<int> columns { 0 };
};

DisplayLine displayLine(const std::string& line)
{
    DisplayLine result;
    constexpr int kTabWidth = 8;
    for (std::size_t position = 0; position < line.size();) {
        uint32_t code;
        std::size_t count;
        bool valid = Unicode::decode(line, position, code, count);
        position += count;
        int width = DiagnosticWidth::cells(code);
        std::string text = Unicode::encode(code);
        if (code == '\t') {
            width = kTabWidth - static_cast<int>(result.cells.size()) % kTabWidth;
            text.assign(width, ' ');
        } else if (!valid || width < 0 || (width == 0 && result.cells.empty())
            || code < 32 || (code >= 0x7F && code <= 0x9F)
            || (Unicode::properties(code) & Unicode::kFormat)) {
            text = escaped(code);
            width = static_cast<int>(text.size());
        }
        if (width == 0 && !result.cells.empty()) {
            auto last = result.cells.size() - 1;
            while (last > 0 && result.cells[last].empty()) { --last; }
            result.cells[last] += text;
        } else if (width > 0) {
            // Tabs and escapes consist of independently printable ASCII cells.
            if (text.size() == static_cast<std::size_t>(width) && width > 2) {
                for (char byte : text) { result.cells.emplace_back(1, byte); }
            } else {
                result.cells.push_back(text);
                for (int i = 1; i < width; ++i) { result.cells.emplace_back(); }
            }
        }
        result.columns.push_back(static_cast<int>(result.cells.size()));
    }
    return result;
}

} // namespace

Diagnostics::Diagnostics(DiagnosticOptions options)
    : m_options(options)
{
    m_files.push_back({ "<unknown>", {}, {} });
#ifdef _WIN32
    if (colored()) {
        HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
        DWORD mode;
        if (GetConsoleMode(handle, &mode)) { SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING); }
    }
#endif
}

int Diagnostics::addFile(std::string name)
{
    m_files.push_back({ std::move(name), {}, {} });
    return static_cast<int>(m_files.size()) - 1;
}

const std::string& Diagnostics::fileName(int file) const
{
    if (file < 0 || file >= static_cast<int>(m_files.size())) { return m_files.front().name; }
    return m_files[static_cast<std::size_t>(file)].name;
}

void Diagnostics::setSource(int file, const std::string& source)
{
    if (file <= 0 || file >= static_cast<int>(m_files.size())) { return; }
    File& entry = m_files[file];
    entry.lines = { "" };
    entry.spans.clear();
    for (std::size_t position = source.starts_with("\xEF\xBB\xBF") ? 3 : 0; position < source.size();) {
        uint32_t code;
        std::size_t count;
        Unicode::decode(source, position, code, count);
        if (code == '\n' || code == 0x85 || code == 0x2028 || code == 0x2029) {
            if (!entry.lines.back().empty() && entry.lines.back().back() == '\r') { entry.lines.back().pop_back(); }
            entry.lines.emplace_back();
        } else {
            entry.lines.back().append(source, position, count);
        }
        position += count;
    }
}

void Diagnostics::addSpan(const SourceLocation& start, const SourceLocation& end)
{
    if (start.file > 0 && start.file < static_cast<int>(m_files.size())) {
        m_files[start.file].spans[{start.line, start.column}] = end;
    }
}

Diagnostics::Context::Context(Diagnostics& diagnostics, SourceLocation location, std::string message)
    : m_diagnostics(diagnostics)
{
    m_diagnostics.m_contexts.push_back({ location, std::move(message) });
}

Diagnostics::Context::~Context()
{
    m_diagnostics.m_contexts.pop_back();
}

void Diagnostics::error(const SourceLocation& location, const std::string& message)
{
    ++m_errorCount;
    m_reported = m_errorCount <= kMaxReportedDiagnostics;
    if (m_reported) {
        report(location, "error", message);
        for (auto context = m_contexts.rbegin(); context != m_contexts.rend(); ++context) {
            if (context->location.file != location.file || context->location.line != location.line
                || context->location.column != location.column) {
                report(context->location, "note", context->message);
            }
        }
    } else if (m_errorCount == kMaxReportedDiagnostics + 1) {
        report(location, "error", "too many errors; further diagnostics suppressed");
    }
}

void Diagnostics::warning(const SourceLocation& location, const std::string& message)
{
    ++m_warningCount;
    m_reported = true;
    report(location, "warning", message);
}

void Diagnostics::note(const SourceLocation& location, const std::string& message)
{
    if (m_reported) { report(location, "note", message); }
}

bool Diagnostics::rich() const
{
    if (m_options.format != DiagnosticOptions::Format::Auto) {
        return m_options.format == DiagnosticOptions::Format::Rich;
    }
    const char* term = std::getenv("TERM");
    return terminal() && (term == nullptr || std::string(term) != "dumb");
}

bool Diagnostics::colored() const
{
    if (m_options.color != DiagnosticOptions::Color::Auto) {
        return m_options.color == DiagnosticOptions::Color::Always;
    }
    const char* term = std::getenv("TERM");
    return terminal() && std::getenv("NO_COLOR") == nullptr && (term == nullptr || std::string(term) != "dumb");
}

void Diagnostics::snippet(const SourceLocation& location, const char* color)
{
    if (location.file <= 0 || location.file >= static_cast<int>(m_files.size())) { return; }
    const File& file = m_files[location.file];
    if (location.line <= 0 || location.line > static_cast<int>(file.lines.size())) { return; }
    SourceLocation end = location;
    auto span = file.spans.find({location.line, location.column});
    if (span != file.spans.end()) { end = span->second; }
    int last = std::clamp(end.line, location.line, static_cast<int>(file.lines.size()));
    if (last > location.line && end.column == 1) { --last; }
    std::string gutter(std::to_string(last).size(), ' ');
    std::cerr << gutter << " --> " << file.name << ':' << location.line << ':' << location.column << '\n';
    std::cerr << gutter << " |\n";
    for (int number = location.line; number <= last; ++number) {
        // Keep arbitrarily long multiline spans bounded while showing both ends.
        if (last - location.line > 3 && number == location.line + 2) {
            std::cerr << gutter << " | ...\n";
            number = last - 1;
            continue;
        }
        DisplayLine line = displayLine(file.lines[number - 1]);
        auto column = [&](int logical) {
            return line.columns[std::clamp(logical - 1, 0, static_cast<int>(line.columns.size()) - 1)];
        };
        int start = number == location.line ? column(location.column) : 0;
        int finish = number == end.line ? column(end.column) : static_cast<int>(line.cells.size());
        finish = std::max(start + 1, finish);
        int available = terminalWidth() - static_cast<int>(gutter.size()) - 10;
        int left = static_cast<int>(line.cells.size()) > available || start >= available
            ? std::max(0, start - available / 3) : 0;
        // Never split a wide scalar at the edge of a clipped window.
        while (left > 0 && left < static_cast<int>(line.cells.size()) && line.cells[left].empty()) { --left; }
        int right = std::min(static_cast<int>(line.cells.size()), left + available);
        while (right > left && right < static_cast<int>(line.cells.size()) && line.cells[right].empty()) { --right; }
        std::string label = std::to_string(number);
        std::cerr << std::string(gutter.size() - label.size(), ' ') << label << " | " << (left > 0 ? "..." : "");
        for (int i = left; i < right; ++i) { std::cerr << line.cells[i]; }
        if (right < static_cast<int>(line.cells.size())) { std::cerr << "..."; }
        std::cerr << '\n' << gutter << " | " << std::string((left > 0 ? 3 : 0) + start - left, ' ');
        if (*color) { std::cerr << color; }
        std::cerr << '^' << std::string(std::max(0, std::min(finish, left + available) - start - 1), '~');
        if (*color) { std::cerr << "\033[0m"; }
        std::cerr << '\n';
    }
}

void Diagnostics::report(const SourceLocation& location, const char* severity, const std::string& message)
{
    const char* color = !colored() ? "" : std::string(severity) == "error" ? "\033[1;31m"
        : std::string(severity) == "warning" ? "\033[1;33m" : "\033[1;36m";
    std::cerr << fileName(location.file);
    if (location.line > 0) { std::cerr << ':' << location.line << ':' << location.column; }
    std::cerr << ": " << color << severity << ": ";
    if (*color) { std::cerr << "\033[0m"; }
    std::cerr << message << '\n';
    if (rich()) { snippet(location, color); }
}
