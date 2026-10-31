#include "../adac/WindowsAbi.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#define CHECK(condition) do { if (!(condition)) { \
    std::cerr << "Windows float ABI check failed at line " << __LINE__ << '\n'; std::abort(); \
} } while (0)

int main(int argc, char** argv)
{
    if (argc == 3) {
        // Exercise the same lowering on another host before amd64_win QBE.
        std::ifstream input(argv[1]);
        std::ofstream output(argv[2]);
        if (!input || !output) { return 1; }
        std::string line, prologue;
        while (std::getline(input, line)) {
            if (line.starts_with("function ") || line.starts_with("export function ")) {
                WindowsParameters incoming = lowerWindowsParameters(line);
                line = incoming.signature;
                prologue = incoming.prologue;
            }
            output << line << '\n';
            if (!prologue.empty() && line.starts_with('@')) {
                output << prologue;
                prologue.clear();
            }
        }
        return output ? 0 : 1;
    }
    const std::string kRegisters = "function d $f(s %a, d %b, s %c, d %d)";
    CHECK(lowerWindowsParameters(kRegisters).signature == kRegisters);
    CHECK(lowerWindowsParameters(kRegisters).prologue.empty());
    WindowsParameters stack = lowerWindowsParameters("export function d $f(l %link, l %to, l %first, l %last, s %x, d %y, w %n)");
    CHECK(stack.signature == "export function d $f(l %link, l %to, l %first, l %last, l %.winabi.bits.4, l %.winabi.bits.5, w %n)");
    CHECK(stack.prologue == "    %.winabi.word.4 =w copy %.winabi.bits.4\n"
        "    %x =s cast %.winabi.word.4\n"
        "    %y =d cast %.winabi.bits.5\n");
    const std::string kEnvironment = "function $f(env %link, s %a, s %b, d %c, s %d)";
    CHECK(lowerWindowsParameters(kEnvironment).signature == kEnvironment);
    CHECK(lowerWindowsParameters(kEnvironment).prologue.empty());
    CHECK(lowerWindowsParameters("function $f()").signature == "function $f()");
    CHECK(lowerWindowsParameters("function $f(l %x)").prologue.empty());
    return 0;
}
