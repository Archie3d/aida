#include "WindowsAbi.h"

WindowsParameters lowerWindowsParameters(std::string_view signature)
{
    WindowsParameters result {std::string(signature), {}};
    std::size_t open = signature.find('(');
    std::size_t close = signature.rfind(')');
    if (open == std::string_view::npos || close == std::string_view::npos || close < open) {
        return result;
    }
    result.signature.assign(signature.substr(0, open + 1));
    std::string_view parameters = signature.substr(open + 1, close - open - 1);
    std::size_t slot = 0;
    while (!parameters.empty()) {
        std::size_t comma = parameters.find(',');
        std::string_view parameter = parameters.substr(0, comma);
        std::size_t start = parameter.find_first_not_of(" \t");
        std::string_view trimmed = start == std::string_view::npos ? std::string_view{} : parameter.substr(start);
        bool floating = trimmed.starts_with("s ") || trimmed.starts_with("d ");
        if (slot >= 4 && floating) {
            std::string name(trimmed.substr(2));
            std::string carrier = "%.winabi.bits." + std::to_string(slot);
            result.signature.append(parameter.substr(0, start));
            result.signature += "l " + carrier;
            if (trimmed.front() == 's') {
                std::string word = "%.winabi.word." + std::to_string(slot);
                result.prologue += "    " + word + " =w copy " + carrier + "\n";
                result.prologue += "    " + name + " =s cast " + word + "\n";
            } else {
                result.prologue += "    " + name + " =d cast " + carrier + "\n";
            }
        } else {
            result.signature.append(parameter);
        }
        // QBE environment parameters do not occupy a Windows argument slot.
        if (!trimmed.starts_with("env ")) { ++slot; }
        if (comma == std::string_view::npos) { break; }
        result.signature += ',';
        parameters.remove_prefix(comma + 1);
    }
    result.signature.append(signature.substr(close));
    return result;
}
