#pragma once

#include "../support/DiagnosticOptions.h"

#include <map>
#include <string>
#include <vector>

// Where something was written.  The file is an index into the table the
// Diagnostics below keeps, so that a location stays meaningful once the units
// of a program and the ones it draws from the library are compiled together.
struct SourceLocation
{
    int line = 0;
    int column = 0; // One-based Unicode scalar column; tabs count as one.
    int file = 0;
};

class Diagnostics
{
public:
    explicit Diagnostics(DiagnosticOptions options = {});

    // Source text is retained from the exact input snapshot, never reread when
    // reporting an error (the file may have changed or be unavailable).
    void setSource(int file, const std::string& source);
    void addSpan(const SourceLocation& start, const SourceLocation& end);
    void note(const SourceLocation& location, const std::string& message);

    class Context
    {
    public:
        Context(Diagnostics& diagnostics, SourceLocation location, std::string message);
        ~Context();
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

    private:
        Diagnostics& m_diagnostics;
    };

    // Records a file and returns the identifier its locations carry.
    int addFile(std::string name);
    const std::string& fileName(int file) const;

    void error(const SourceLocation& location, const std::string& message);
    void warning(const SourceLocation& location, const std::string& message);

    bool hasErrors() const { return m_errorCount > 0; }
    int errorCount() const { return m_errorCount; }

private:
    void report(const SourceLocation& location, const char* severity, const std::string& message);

    void snippet(const SourceLocation& location, const char* color);
    bool rich() const;
    bool colored() const;

    struct File
    {
        std::string name;
        std::vector<std::string> lines;
        std::map<std::pair<int, int>, SourceLocation> spans;
    };
    struct Related
    {
        SourceLocation location;
        std::string message;
    };
    DiagnosticOptions m_options;
    std::vector<File> m_files;
    std::vector<Related> m_contexts;
    bool m_reported = false;
    static constexpr int kMaxReportedDiagnostics = 100;
    int m_errorCount = 0;
    int m_warningCount = 0;
};
