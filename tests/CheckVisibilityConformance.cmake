cmake_minimum_required(VERSION 3.20)
set(work "${WORKDIR}/visibility-conformance")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${work}")
set(math_library)
if(SEPARATE_LIBM)
    set(math_library -lm)
endif()

function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Command failed: ${ARGV}\n${status}\n${output}${errors}")
    endif()
    set(actual "${output}" PARENT_SCOPE)
endfunction()

function(reject diagnostic)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status EQUAL 1 OR NOT errors MATCHES "${diagnostic}")
        message(FATAL_ERROR "Expected '${diagnostic}': ${ARGN}\n${status}\n${output}${errors}")
    endif()
endfunction()

function(build_modes)
    run("${ADAC}" -o "${work}/first.ssa" "${work}/audit_main.adb")
    run("${ADAC}" -o "${work}/second.ssa" "${work}/audit_main.adb")
    run("${CMAKE_COMMAND}" -E compare_files "${work}/first.ssa" "${work}/second.ssa")
    run("${ADA}" -D "${work}/objects" -o "${work}/apart" "${work}/audit_main.adb")
    run("${work}/apart")
    if(NOT actual STREQUAL "visibility cross-unit checks passed\n")
        message(FATAL_ERROR "Unexpected separate output: ${actual}")
    endif()
    run("${ADA}" -S -o "${work}/whole.s" "${work}/audit_main.adb")
    run("${CC}" -pthread "${work}/whole.s" "${RUNTIME}" ${math_library} -o "${work}/whole")
    run("${work}/whole")
    if(NOT actual STREQUAL "visibility cross-unit checks passed\n")
        message(FATAL_ERROR "Unexpected whole output: ${actual}")
    endif()
endfunction()

file(WRITE "${work}/values.ads" [=[package Values is
    Value : constant := 4;
end Values;
]=])
file(WRITE "${work}/hidden_names.ads" "package Hidden_Names is\n    Hidden_Value : constant := 3;\nend Hidden_Names;\n")
file(WRITE "${work}/parent.ads" [=[with Values; use Values;
with Hidden_Names;
package Parent is
    subtype Small is Integer range 1 .. 10;
    type Hidden is private;
    function Pick (X : Small := Value) return Small;
private
    use Hidden_Names;
    type Hidden is record
        Value : Integer;
    end record;
    Secret : constant := Hidden_Value;
end Parent;
]=])
file(WRITE "${work}/parent.adb" [=[package body Parent is
    function Pick (X : Small := Values.Value) return Small is
    begin
        return X;
    end Pick;
end Parent;
]=])
file(WRITE "${work}/parent-child.ads" [=[package Parent.Child is
    function Get return Integer;
private
    Copy : constant := Hidden_Value;
end Parent.Child;
]=])
file(WRITE "${work}/parent-child.adb" [=[package body Parent.Child is
    function Get return Integer is
    begin
        return Pick + Secret + Copy;
    end Get;
end Parent.Child;
]=])
file(WRITE "${work}/parent-read.ads" "function Parent.Read (X : Integer) return Integer;\n")
file(WRITE "${work}/parent-read.adb" "function Parent.Read (X : Integer) return Integer is\nbegin return X + Secret; end Parent.Read;\n")
file(WRITE "${work}/audit_main.adb" [=[with Ada.Text_IO; use Ada.Text_IO;
with Parent.Child, Parent.Read;
procedure Audit_Main is
begin
    if Parent.Pick /= 4 or Parent.Child.Get /= 10 then
        raise Program_Error;
    end if;
    if Parent.Read (2) /= 5 then raise Program_Error; end if;
    Put_Line ("visibility cross-unit checks passed");
end Audit_Main;
]=])
build_modes()
# Dependency edits must still rebuild a client using an inherited default.
file(READ "${work}/values.ads" values)
string(REPLACE ":= 4" ":= 5" values "${values}")
file(WRITE "${work}/values.ads" "${values}")
file(READ "${work}/audit_main.adb" main)
string(REPLACE "/= 4" "/= 5" main "${main}")
string(REPLACE "/= 10" "/= 11" main "${main}")
file(WRITE "${work}/audit_main.adb" "${main}")
build_modes()

# Public child specifications cannot expose a parent's private declarations.
file(WRITE "${work}/parent-bad.ads" "package Parent.Bad is\n    Value : Integer := Secret;\nend Parent.Bad;\n")
reject("'Secret' is not declared" "${ADAC}" -o "${work}/bad.ssa" "${work}/parent-bad.ads")
reject("'Secret' is not declared" "${ADAC}" -c -D "${work}/badobj" "${work}/parent-bad.ads")
file(WRITE "${work}/parent-bad.ads" "package Parent.Bad is\n    Value : Integer := Hidden_Value;\nend Parent.Bad;\n")
reject("'Hidden_Value' is not declared" "${ADAC}" -o "${work}/bad.ssa" "${work}/parent-bad.ads")
file(WRITE "${work}/parent-bad.ads" "package Parent.Bad is\n    package Nested is\n    private\n        X : Hidden := (Value => 1);\n    end Nested;\nend Parent.Bad;\n")
reject("whose representation" "${ADAC}" -o "${work}/bad.ssa" "${work}/parent-bad.ads")
# Loading another child does not make it visible without a with clause.
file(WRITE "${work}/outsider.adb" "with Parent;\nprocedure Outsider is\n    X : Integer := Parent.Child.Get;\nbegin null; end Outsider;\n")
reject("'Child' is not declared" "${ADAC}" -o "${work}/bad.ssa" "${work}/parent-child.ads" "${work}/outsider.adb")
# A use clause in one item of a bundle must not leak to its neighbour.
file(WRITE "${work}/context.adb" [=[with Values; use Values;
with Hidden_Names;
package First is
    X : Integer := Value;
end First;
procedure Second is
    X : Integer := Value;
begin null; end Second;
]=])
reject("'Value' is not declared" "${ADAC}" -o "${work}/bad.ssa" "${work}/context.adb")
reject("'Value' is not declared" "${ADAC}" -c -D "${work}/contextobj" "${work}/context.adb")
# Explicit source order must not grant visibility either.
file(WRITE "${work}/unmentioned.adb" "procedure Unmentioned is\n    X : Integer := Values.Value;\nbegin null; end Unmentioned;\n")
reject("'Values' is not declared" "${ADAC}" -o "${work}/bad.ssa" "${work}/values.ads" "${work}/unmentioned.adb")
reject("'Values' is not declared" "${ADAC}" -o "${work}/bad.ssa" "${work}/unmentioned.adb" "${work}/values.ads")

# A profile edit is diagnosed during scanning, before stale artifacts can link.
file(READ "${work}/parent.adb" body)
string(REPLACE "X : Small" "Other : Small" badbody "${body}")
file(WRITE "${work}/parent.adb" "${badbody}")
reject("does not conform" "${ADAC}" -o "${work}/bad.ssa" "${work}/audit_main.adb")
reject("does not conform" "${ADAC}" -c -D "${work}/badobj" "${work}/parent.adb")
reject("does not conform" "${ADA}" -D "${work}/objects" -o "${work}/apart" "${work}/audit_main.adb")
file(WRITE "${work}/parent.adb" "${body}")
build_modes()

# Missing library subprogram and package completions fail before the linker.
file(WRITE "${work}/missing.ads" "procedure Missing (X : Integer);\n")
file(WRITE "${work}/needs.ads" "package Needs is\n    procedure Action;\nend Needs;\n")
file(WRITE "${work}/incomplete.adb" "with Missing, Needs;\nprocedure Incomplete is\nbegin Missing (1); Needs.Action; end Incomplete;\n")
reject("subprogram 'Missing' has no body" "${ADAC}" -o "${work}/bad.ssa" "${work}/incomplete.adb")
reject("subprogram 'Action' has no body" "${ADA}" -o "${work}/incomplete" "${work}/incomplete.adb")
# Specification-only compilation may legitimately precede the body.
run("${ADAC}" -c -D "${work}/specobj" "${work}/missing.ads")
file(WRITE "${work}/missing.adb" "procedure Missing (X : Integer) is begin null; end Missing;\n")
file(WRITE "${work}/needs.adb" "package body Needs is\n    procedure Action is null;\nend Needs;\n")
run("${ADA}" -o "${work}/incomplete" "${work}/incomplete.adb")
run("${work}/incomplete")
# Bodyless packages, imports and null procedures are valid completions.
file(WRITE "${work}/bodyless.ads" [=[package Bodyless is
    Value : constant := 1;
    procedure Done is null;
    procedure Foreign;
    pragma Import (C, Foreign, "foreign_not_called");
end Bodyless;
]=])
file(WRITE "${work}/bodyless_main.adb" "with Bodyless;\nprocedure Bodyless_Main is begin Bodyless.Done; end Bodyless_Main;\n")
run("${ADA}" -o "${work}/bodyless_main" "${work}/bodyless_main.adb")
run("${work}/bodyless_main")

file(WRITE "${work}/system_visibility.adb" "procedure System_Visibility is\n    X : System.Address;\nbegin null; end System_Visibility;\n")
reject("'system.address' is not declared" "${ADAC}" -o "${work}/bad.ssa" "${work}/system_visibility.adb")
file(WRITE "${work}/missing_generic.ads" "generic\nfunction Missing_Generic return Integer;\n")
reject("generic subprogram 'Missing_Generic' has no body" "${ADAC}" -o "${work}/bad.ssa" "${work}/missing_generic.ads")
run("${ADAC}" -c -D "${work}/specobj" "${work}/missing_generic.ads")

# Context use clauses do not make names visible in other context clauses.
file(WRITE "${work}/containers.ads" "package Containers is\n    package Nested is end Nested;\nend Containers;\n")
file(WRITE "${work}/context_use.adb" "with Containers; use Containers; use Nested;\nprocedure Context_Use is begin null; end Context_Use;\n")
reject("unknown package 'Nested'" "${ADAC}" -o "${work}/bad.ssa" "${work}/context_use.adb")
