# Library renamed views are complete units and cannot introduce descendants.
file(MAKE_DIRECTORY "${WORKDIR}")
function(reject name source expected)
    file(WRITE "${WORKDIR}/${name}" "${source}")
    execute_process(COMMAND "${ADAC}" -I "${SOURCES}" -D "${WORKDIR}/units"
        "${WORKDIR}/${name}" RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 20)
    string(FIND "${errors}" "${expected}" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "Incorrect renaming diagnostic for ${name}: ${status}\n${output}${errors}")
    endif()
endfunction()
# Each source directory is isolated so an earlier deliberately invalid body
# cannot become an accidental dependency of a later scenario.
set(base "${WORKDIR}")
set(WORKDIR "${base}/package-body")
file(MAKE_DIRECTORY "${WORKDIR}")
file(WRITE "${WORKDIR}/alias.ads" "with Renaming_Model; package Alias renames Renaming_Model;\n")
reject(alias.adb "package body Alias is end Alias;\n" "is already complete; a body is not allowed")
set(WORKDIR "${base}/generic-body")
file(MAKE_DIRECTORY "${WORKDIR}")
file(WRITE "${WORKDIR}/alias.ads" "with Renaming_Source; generic function Alias renames Renaming_Source;\n")
reject(alias.adb "function Alias (Value : Integer := 2) return Integer is begin return Value; end Alias;\n"
    "is already complete; a body is not allowed")
set(WORKDIR "${base}/child")
file(MAKE_DIRECTORY "${WORKDIR}")
file(WRITE "${WORKDIR}/alias.ads" "with Renaming_Model; package Alias renames Renaming_Model;\n")
reject(alias-child.ads "package Alias.Child is end Alias.Child;\n"
    "a renamed package cannot be the parent of a child unit")
set(WORKDIR "${base}/nested")
file(MAKE_DIRECTORY "${WORKDIR}")
reject(alias.ads "with Renaming_Model; package Alias renames Renaming_Model.Nested;\n"
    "a library unit renaming must rename a library unit")
set(WORKDIR "${base}/generic-nested")
file(MAKE_DIRECTORY "${WORKDIR}")
reject(alias.ads "with Renaming_Model; generic function Alias renames Renaming_Model.Factory;\n"
    "a library unit renaming must rename a library unit")
