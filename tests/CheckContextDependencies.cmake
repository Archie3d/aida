set(work "${WORKDIR}/context-dependencies")
file(MAKE_DIRECTORY "${work}")
file(WRITE "${work}/dep_a.ads" "limited with Dep_B;
package Dep_A is
    type Pointer is access Dep_B.Item;
    type Item is null record;
    function Make return Pointer;
end Dep_A;
")
file(WRITE "${work}/dep_a.adb" "with Dep_B;
package body Dep_A is
    function Make return Pointer is
    begin
        return new Dep_B.Item;
    end Make;
end Dep_A;
")
file(WRITE "${work}/dep_b.ads" "limited with Dep_A;
package Dep_B is
    type Pointer is access Dep_A.Item;
    type Item is record
        Value : Integer := 7;
    end record;
    function Read (Object : Item) return Integer;
end Dep_B;
")
file(WRITE "${work}/dep_b.adb" "package body Dep_B is
    function Read (Object : Item) return Integer is
    begin
        return Object.Value;
    end Read;
end Dep_B;
")
file(WRITE "${work}/context_main.adb" "with Ada.Text_IO; use Ada.Text_IO;
with Dep_A;
with Dep_B;
procedure Context_Main is
    Object : Dep_A.Pointer := Dep_A.Make;
begin
    Put_Line (Integer'Image (Dep_B.Read (Object.all)));
end Context_Main;
")
function(build_run expected)
    execute_process(COMMAND "${ADA}" -D "${work}/objects" -o "${work}/program" "${work}/context_main.adb"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 30)
    if(NOT status STREQUAL "0")
        message(FATAL_ERROR "Context dependency build failed: ${status}\n${output}${errors}")
    endif()
    execute_process(COMMAND "${work}/program" RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 10)
    if(NOT status STREQUAL "0" OR NOT errors STREQUAL "" OR NOT output STREQUAL " ${expected}\n")
        message(FATAL_ERROR "Stale context dependency executable: ${status}\n${output}${errors}")
    endif()
endfunction()
build_run(7)
file(READ "${work}/objects/units.manifest" manifest)
foreach(pair "dep_a;dep_b" "dep_b;dep_a")
    list(GET pair 0 client)
    list(GET pair 1 dependency)
    string(FIND "${manifest}" "part ${client}.spec\nlimited ${dependency}.spec\n" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Limited dependency was lost or became an elaboration edge:\n${manifest}")
    endif()
endforeach()
# The binder checks partition membership without imposing limited-view order.
string(REPLACE "limited dep_b.spec" "limited missing.spec" invalid_manifest "${manifest}")
file(WRITE "${work}/objects/units.manifest" "${invalid_manifest}")
execute_process(COMMAND "${ADAC}" --bind context_main -D "${work}/objects"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
string(FIND "${errors}" "missing limited-view dependency" missing)
if(NOT status STREQUAL "1" OR missing EQUAL -1)
    message(FATAL_ERROR "The binder did not validate a missing limited dependency: ${status}\n${output}${errors}")
endif()
file(WRITE "${work}/objects/units.manifest" "${manifest}")
file(READ "${work}/objects/dep_a.ali" before)
file(READ "${work}/dep_b.ads" source)
string(REPLACE ":= 7" ":= 8" source "${source}")
file(WRITE "${work}/dep_b.ads" "${source}")
build_run(8)
file(READ "${work}/objects/dep_a.ali" after)
if(before STREQUAL after)
    message(FATAL_ERROR "Changing a limited dependency's specification did not invalidate its client")
endif()
file(READ "${work}/dep_b.adb" source)
string(REPLACE "return Object.Value;" "return Object.Value + 10;" source "${source}")
file(WRITE "${work}/dep_b.adb" "${source}")
build_run(18)
file(READ "${work}/objects/dep_a.ali" body_after)
if(NOT after STREQUAL body_after)
    message(FATAL_ERROR "An ordinary dependency body unnecessarily invalidated a limited-view client")
endif()
# One limited edge still breaks the semantic cycle; two ordinary edges do not.
file(READ "${work}/dep_a.ads" source)
string(REPLACE "limited with" "with" source "${source}")
file(WRITE "${work}/dep_a.ads" "${source}")
build_run(18)
file(READ "${work}/dep_b.ads" source)
string(REPLACE "limited with" "with" source "${source}")
file(WRITE "${work}/dep_b.ads" "${source}")
execute_process(COMMAND "${ADAC}" --scan -D "${work}/objects" "${work}/context_main.adb"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
string(FIND "${errors}" "library dependency cycle" found)
if(NOT status STREQUAL "1" OR found EQUAL -1)
    message(FATAL_ERROR "An ordinary semantic cycle was accepted or misdiagnosed: ${status}\n${output}${errors}")
endif()
