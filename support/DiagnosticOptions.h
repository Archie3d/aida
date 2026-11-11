#pragma once

#include <string>

// Shared by the driver and frontend so both accept the same rendering options.
struct DiagnosticOptions
{
    enum class Format { Auto, Plain, Rich };
    enum class Color { Auto, Always, Never };

    Format format = Format::Auto;
    Color color = Color::Auto;

    static bool recognizes(const std::string& argument)
    {
        return argument.starts_with("--diagnostic-format=") || argument.starts_with("--color=");
    }

    bool parse(const std::string& argument)
    {
        if (argument == "--diagnostic-format=auto") { format = Format::Auto; }
        else if (argument == "--diagnostic-format=plain") { format = Format::Plain; }
        else if (argument == "--diagnostic-format=rich") { format = Format::Rich; }
        else if (argument == "--color=auto") { color = Color::Auto; }
        else if (argument == "--color=always") { color = Color::Always; }
        else if (argument == "--color=never") { color = Color::Never; }
        else { return false; }
        return true;
    }
};
