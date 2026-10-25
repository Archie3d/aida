set(work "${WORKDIR}/subunit-checks")
file(MAKE_DIRECTORY "${work}")

# Each root is an independent environment, so a failing body cannot be found
# accidentally through the source search path of another root.
function(reject name source diagnostic)
    set(directory "${work}/${name}")
    file(MAKE_DIRECTORY "${directory}")
    file(WRITE "${directory}/root.adb" "${source}\n")
    execute_process(COMMAND "${ADAC}" -o "${directory}/root.ssa" "${directory}/root.adb"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
    string(FIND "${errors}" "${diagnostic}" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "${name}: expected ${diagnostic}, got ${status}\n${output}${errors}")
    endif()
endfunction()

reject(missing "procedure Root is procedure Work is separate; begin null; end Root;"
    "cannot find subunit 'root.work'")
reject(orphan "procedure Root is begin null; end Root;
separate (Root) procedure Work is begin null; end Work;"
    "subunit has no corresponding body stub")
reject(parent_missing "separate (Missing) procedure Work is begin null; end Work;"
    "cannot find parent body")
reject(wrong_parent "procedure Root is procedure Work is separate; begin null; end Root;
separate (Other) procedure Work is begin null; end Work;"
    "cannot find subunit 'root.work'")
reject(wrong_name "procedure Root is procedure Work is separate; begin null; end Root;
separate (Root) procedure Other is begin null; end Other;"
    "cannot find subunit 'root.work'")
reject(duplicate "procedure Root is procedure Work is separate; begin null; end Root;
separate (Root) procedure Work is begin null; end Work;
separate (Root) procedure Work is begin null; end Work;"
    "duplicate subunit 'root.work'")
reject(kind "procedure Root is procedure Work is separate; begin null; end Root;
separate (Root) package body Work is end Work;"
    "subunit kind does not match")
reject(profile_type "procedure Root is procedure Work (X : Integer) is separate; begin null; end Root;
separate (Root) procedure Work (X : Float) is begin null; end Work;"
    "subunit profile does not conform")
reject(profile_name "procedure Root is procedure Work (X : Integer) is separate; begin null; end Root;
separate (Root) procedure Work (Y : Integer) is begin null; end Work;"
    "subprogram body does not conform")
reject(profile_mode "procedure Root is procedure Work (X : in out Integer) is separate; begin null; end Root;
separate (Root) procedure Work (X : Integer) is begin null; end Work;"
    "subprogram body does not conform")
reject(profile_default "procedure Root is procedure Work (X : Integer := 1) is separate; begin null; end Root;
separate (Root) procedure Work (X : Integer := 2) is begin null; end Work;"
    "subprogram body does not conform")
reject(profile_result "procedure Root is function Work return Integer is separate; begin null; end Root;
separate (Root) function Work return Float is begin return 1.0; end Work;"
    "subunit profile does not conform")
reject(stub_declaration "procedure Root is procedure Work (X : Integer);
procedure Work (Y : Integer) is separate; begin null; end Root;
separate (Root) procedure Work (Y : Integer) is begin null; end Work;"
    "subprogram body does not conform")
reject(distinct "procedure Root is procedure Work (X : Integer) is separate;
procedure Work (X : Float) is separate; begin null; end Root;"
    "body stub names must be distinct")
reject(too_late "procedure Root is procedure Work is separate;
Later : Integer := 1; begin null; end Root;
separate (Root) procedure Work is begin Later := 2; end Work;"
    "'Later' is not declared")
reject(outer_spec "package Work is end Work;
with Work;
procedure Root is package body Work is separate; begin null; end Root;
separate (Root) package body Work is end Work;"
    "package body stub must complete a declaration in the same declarative region")
reject(no_spec "procedure Root is package body Work is separate; begin null; end Root;
separate (Root) package body Work is end Work;"
    "has no specification")
reject(context_isolation "package Secret is X : Integer := 1; end Secret;
procedure Root is procedure Work is separate;
begin Secret.X := 2; end Root;
with Secret;
separate (Root) procedure Work is begin Secret.X := 3; end Work;"
    "'Secret' is not declared")
reject(sibling_isolation "package Secret is X : Integer := 1; end Secret;
procedure Root is procedure Work is separate; procedure Other is separate; begin null; end Root;
with Secret;
separate (Root) procedure Work is begin Secret.X := 3; end Work;
separate (Root) procedure Other is begin Secret.X := 4; end Other;"
    "'Secret' is not declared")
reject(limited_context "package Secret is end Secret;
procedure Root is procedure Work is separate; begin null; end Root;
limited with Secret;
separate (Root) procedure Work is begin null; end Work;"
    "a limited with clause is not allowed on a body")
reject(null_body "procedure Root is procedure Work is separate; begin null; end Root;
separate (Root) procedure Work is null;"
    "a subunit requires a proper")
reject(nested_body "procedure Root is procedure Nested is procedure Work is separate;
begin null; end Nested; begin null; end Root;"
    "a body stub must appear immediately within a compilation unit body")
reject(block "procedure Root is begin declare procedure Work is separate; begin null; end; end Root;"
    "a body stub must appear immediately within a compilation unit body")
reject(spec "package Root is procedure Work is separate; end Root;"
    "a body stub must appear immediately within a compilation unit body")

# Incremental builds include every transitive subunit in the parent's digest.
set(directory "${work}/incremental")
file(MAKE_DIRECTORY "${directory}")
file(WRITE "${directory}/root.adb" "with Ada.Text_IO; use Ada.Text_IO;
procedure Root is function Value return Integer is separate;
begin Put_Line (Integer'Image (Value)); end Root;\n")
file(WRITE "${directory}/root-value.adb" "separate (Root)
function Value return Integer is function Deep return Integer is separate;
begin return Deep; end Value;\n")
file(WRITE "${directory}/root-value-deep.adb" "separate (Root.Value)
function Deep return Integer is begin return 7; end Deep;\n")
function(build_run expected)
    execute_process(COMMAND "${ADA}" -D "${directory}/objects" -o "${directory}/program" "${directory}/root.adb"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status STREQUAL "0")
        message(FATAL_ERROR "Subunit build failed: ${status}\n${output}${errors}")
    endif()
    execute_process(COMMAND "${directory}/program" RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 10)
    if(NOT status STREQUAL "0" OR NOT output STREQUAL " ${expected}\n" OR NOT errors STREQUAL "")
        message(FATAL_ERROR "Stale subunit executable: ${status}\n${output}${errors}")
    endif()
endfunction()
build_run(7)
file(READ "${directory}/objects/units.manifest" manifest)
foreach(source root-value.adb root-value-deep.adb)
    string(FIND "${manifest}" "source ${directory}/${source}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Subunit omitted from parent dependencies: ${manifest}")
    endif()
endforeach()
string(FIND "${manifest}" "unit root-value" standalone)
if(NOT standalone EQUAL -1)
    message(FATAL_ERROR "A subunit incorrectly became a library object")
endif()
file(READ "${directory}/objects/root.ali" before)
file(READ "${directory}/root-value-deep.adb" source)
string(REPLACE "return 7" "return 8" source "${source}")
file(WRITE "${directory}/root-value-deep.adb" "${source}")
build_run(8)
file(READ "${directory}/objects/root.ali" after)
if(before STREQUAL after)
    message(FATAL_ERROR "Changing a nested subunit did not invalidate its parent")
endif()

# Register a custom-named subunit before its parent, and compile that subunit
# directly too. Both must select the parent's object group.
file(WRITE "${directory}/custom.ada" "separate (Root)
function Value return Integer is begin return 9; end Value;\n")
foreach(target root.adb custom.ada)
    execute_process(COMMAND "${ADAC}" -c -D "${directory}/explicit-${target}"
        "${directory}/custom.ada" "${directory}/root.adb" "${directory}/${target}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 20)
    if(NOT status STREQUAL "0" OR NOT EXISTS "${directory}/explicit-${target}/root.ssa")
        message(FATAL_ERROR "Explicit subunit order/target failed: ${status}\n${output}${errors}")
    endif()
endforeach()

# Generic clients depend on the full template, including its subunit sources.
set(directory "${work}/generic-incremental")
file(MAKE_DIRECTORY "${directory}")
file(WRITE "${directory}/numbers.ads" "generic type Item is range <>;
package Numbers is function Value return Item; end Numbers;\n")
file(WRITE "${directory}/numbers.adb" "package body Numbers is
function Value return Item is separate; end Numbers;\n")
file(WRITE "${directory}/numbers-value.adb" "separate (Numbers)
function Value return Item is begin return 7; end Value;\n")
file(WRITE "${directory}/root.adb" "with Ada.Text_IO; use Ada.Text_IO; with Numbers;
procedure Root is package Instance is new Numbers (Integer);
begin Put_Line (Integer'Image (Instance.Value)); end Root;\n")
build_run(7)
file(READ "${directory}/objects/root.ali" before)
file(READ "${directory}/numbers-value.adb" source)
string(REPLACE "return 7" "return 8" source "${source}")
file(WRITE "${directory}/numbers-value.adb" "${source}")
build_run(8)
file(READ "${directory}/objects/root.ali" after)
if(before STREQUAL after)
    message(FATAL_ERROR "A generic subunit edit did not invalidate its client")
endif()

# A library name introduced by a subunit context cannot hide a parent local.
set(directory "${work}/context-shadowing")
file(MAKE_DIRECTORY "${directory}")
file(WRITE "${directory}/shadow.ads" "package Shadow is X : Integer := 1; end Shadow;\n")
file(WRITE "${directory}/root.adb" "with Ada.Text_IO; use Ada.Text_IO;
procedure Root is package Shadow is X : Integer := 2; end Shadow;
function Value return Integer is separate;
begin Put_Line (Integer'Image (Value)); end Root;\n")
file(WRITE "${directory}/root-value.adb" "with Shadow; use Shadow;
separate (Root) function Value return Integer is begin return X; end Value;\n")
build_run(2)
