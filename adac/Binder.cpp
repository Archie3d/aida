#include "Binder.h"

std::string unitTag(const std::string& key)
{
    std::string tag;
    for (char c : key) {
        bool usable = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        tag.push_back(usable ? c : '_');
    }
    return tag;
}

std::string elaborationName(const std::string& key)
{
    return "ada_elab." + unitTag(key);
}

void emitBinder(const std::vector<std::string>& units, const std::string& mainName, std::ostream& out)
{
    int temp = 0;
    int label = 0;
    std::string unhandled = "@unhandled";

    out << "\nexport function w $main(w %argc, l %argv) {\n@start\n";
    out << "    call $__ada_command_line_init(w %argc, l %argv)\n";

    // A unit whose elaboration failed leaves the ones after it unelaborated, so
    // the program stops at the first failure rather than running on.
    for (const std::string& unit : units) {
        std::string pending = "%.t" + std::to_string(temp++);
        std::string raised = "%.t" + std::to_string(temp++);
        std::string next = "@elaborated." + std::to_string(label++);
        out << "    call $" << elaborationName(unit) << "()\n";
        out << "    " << pending << " =l loadl $__ada_exception\n";
        out << "    " << raised << " =w cnel " << pending << ", 0\n";
        out << "    jnz " << raised << ", " << unhandled << ", " << next << "\n";
        out << next << "\n";
    }

    if (!mainName.empty()) {
        out << "    call " << mainName << "()\n";
    }

    std::string pending = "%.t" + std::to_string(temp++);
    std::string raised = "%.t" + std::to_string(temp++);
    out << "    " << pending << " =l loadl $__ada_exception\n";
    out << "    " << raised << " =w cnel " << pending << ", 0\n";
    out << "    jnz " << raised << ", " << unhandled << ", @done\n";

    std::string occurrence = "%.t" + std::to_string(temp++);
    out << unhandled << "\n";
    out << "    " << occurrence << " =l loadl $__ada_exception\n";
    out << "    call $__ada_unhandled(l " << occurrence << ")\n";
    out << "    ret 1\n";

    out << "@done\n";
    out << "    %.exitStatus =w call $__ada_get_exit_status()\n";
    out << "    ret %.exitStatus\n";
    out << "}\n\n";
}
