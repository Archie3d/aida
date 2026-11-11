#include "Diagnostics.h"

#include <string>

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--limit") {
        Diagnostics diagnostics;
        int file = diagnostics.addFile("limit.adb");
        for (int i = 0; i < 105; ++i) {
            diagnostics.error({1, 1, file}, "bounded error");
            diagnostics.note({2, 1, file}, "related declaration");
        }
        return diagnostics.errorCount() == 105 ? 0 : 1;
    }
    DiagnosticOptions options;
    for (int i = 1; i < argc; ++i) {
        if (!options.parse(argv[i])) { return 2; }
    }
    Diagnostics diagnostics(options);
    int file = diagnostics.addFile("sample.adb");
    diagnostics.setSource(file, "\xEF\xBB\xBF\t界é := \"bad\";\r\nsecond line\nthird\nfourth\nfifth\nsixth\n"
        + std::string(150, ' ') + "Offending_Token\n\033[31m control\nlast\n");
    diagnostics.addSpan({1, 2, file}, {1, 5, file});
    diagnostics.error({1, 2, file}, "unicode span");
    diagnostics.note({2, 1, file}, "related declaration");
    diagnostics.addSpan({2, 3, file}, {6, 4, file});
    diagnostics.warning({2, 3, file}, "multiline span");
    diagnostics.addSpan({7, 151, file}, {7, 166, file});
    diagnostics.error({7, 151, file}, "clipped span");
    diagnostics.error({8, 1, file}, "escaped control");
    diagnostics.error({9, 99, file}, "past end of line");
    diagnostics.error({99, 1, file}, "unavailable line");
    diagnostics.error({1, 1, 9999}, "unavailable file");
    int replay = diagnostics.addFile("removed.ads");
    diagnostics.error({4, 2, replay}, "source-free interface");
    {
        Diagnostics::Context outer(diagnostics, {2, 1, file}, "outer instance");
        Diagnostics::Context inner(diagnostics, {3, 1, file}, "inner instance");
        diagnostics.error({4, 1, file}, "nested error");
    }
    diagnostics.error({10, 1, file}, "end of file");
    return diagnostics.errorCount() == 9 ? 0 : 1;
}
