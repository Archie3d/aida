#include "Binder.h"

#include <unordered_map>
#include <unordered_set>

std::string validateElaborationOrder(const std::vector<UnitRecord>& units,
    const std::vector<std::string>& elaborations)
{
    std::unordered_set<std::string> unitKeys;
    std::unordered_map<std::string, const UnitPartRecord*> parts;
    for (const UnitRecord& unit : units) {
        if (unit.key.empty() || !unitKeys.insert(unit.key).second) {
            return "invalid or duplicate unit '" + unit.key + "' in manifest";
        }
        if (unit.parts.empty()) {
            return "missing elaboration parts for unit '" + unit.key + "'";
        }
        for (const UnitPartRecord& part : unit.parts) {
            if (part.key.empty() || (!part.key.ends_with(".spec") && !part.key.ends_with(".body"))) {
                return "invalid elaboration part '" + part.key + "' for unit '" + unit.key + "'";
            }
            if (!parts.emplace(part.key, &part).second) {
                return "duplicate elaboration part '" + part.key + "'";
            }
        }
    }
    std::unordered_map<std::string, std::size_t> positions;
    for (std::size_t i = 0; i < elaborations.size(); ++i) {
        const std::string& key = elaborations[i];
        if (!parts.contains(key)) {
            return "unknown elaboration entry '" + key + "'";
        }
        if (!positions.emplace(key, i).second) {
            return "duplicate elaboration entry '" + key + "'";
        }
    }
    // Traverse in manifest order so diagnostics never depend on hash order.
    for (const UnitRecord& unit : units) {
        for (const UnitPartRecord& part : unit.parts) {
            if (!positions.contains(part.key)) {
                return "missing elaboration entry '" + part.key + "'";
            }
        }
    }
    for (const UnitRecord& unit : units) {
        for (const UnitPartRecord& part : unit.parts) {
            // These edges follow from unit identity, even if a malformed
            // manifest omits them from its explicit dependency lists.
            std::string name = part.key.substr(0, part.key.size() - 5);
            std::vector<std::string> required;
            if (part.key.ends_with(".body") && parts.contains(name + ".spec")) {
                required.push_back(name + ".spec");
            }
            std::size_t parent = name.find_last_of('-');
            if (parent != std::string::npos) {
                required.push_back(name.substr(0, parent) + ".spec");
            }
            for (const std::string& dependency : required) {
                if (!positions.contains(dependency)) {
                    return "missing required elaboration part '" + dependency + "' for '" + part.key + "'";
                }
                if (positions.at(dependency) >= positions.at(part.key)) {
                    return "invalid elaboration order: '" + dependency + "' must precede '" + part.key + "'";
                }
            }
            for (const std::string& dependency : part.dependencies) {
                if (!positions.contains(dependency)) {
                    return "unknown elaboration dependency '" + dependency + "' of '" + part.key + "'";
                }
                if (positions.at(dependency) >= positions.at(part.key)) {
                    return "invalid elaboration order: '" + dependency + "' must precede '" + part.key + "'";
                }
            }
        }
    }
    return {};
}

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
    out << "    %.taskContext =l call $__ada_task_context()\n";
    out << "    call $__ada_command_line_init(w %argc, l %argv)\n";
    out << "    call $__ada_library_enter()\n";

    // A unit whose elaboration failed leaves the ones after it unelaborated, so
    // the program stops at the first failure rather than running on.
    for (const std::string& unit : units) {
        std::string pending = "%.t" + std::to_string(temp++);
        std::string raised = "%.t" + std::to_string(temp++);
        std::string next = "@elaborated." + std::to_string(label++);
        out << "    call $" << elaborationName(unit) << "()\n";
        out << "    " << pending << " =l loadl %.taskContext\n";
        out << "    " << raised << " =w cnel " << pending << ", 0\n";
        out << "    jnz " << raised << ", @shutdown, " << next << "\n";
        out << next << "\n";
    }

    out << "    call $__ada_library_activate()\n";
    out << "    %.activationException =l loadl %.taskContext\n";
    out << "    %.activationFailed =w cnel %.activationException, 0\n";
    out << "    jnz %.activationFailed, @shutdown, @activated\n@activated\n";
    if (!mainName.empty()) {
        out << "    call " << mainName << "()\n";
    }

    out << "    jmp @shutdown\n@shutdown\n";
    out << "    call $__ada_library_finalize()\n";

    std::string pending = "%.t" + std::to_string(temp++);
    std::string raised = "%.t" + std::to_string(temp++);
    out << "    " << pending << " =l loadl %.taskContext\n";
    out << "    " << raised << " =w cnel " << pending << ", 0\n";
    out << "    jnz " << raised << ", " << unhandled << ", @done\n";

    std::string occurrence = "%.t" + std::to_string(temp++);
    out << unhandled << "\n";
    out << "    " << occurrence << " =l loadl %.taskContext\n";
    out << "    call $__ada_unhandled(l " << occurrence << ")\n";
    out << "    ret 1\n";

    out << "@done\n";
    out << "    %.exitStatus =w call $__ada_get_exit_status()\n";
    out << "    ret %.exitStatus\n";
    out << "}\n\n";
}
