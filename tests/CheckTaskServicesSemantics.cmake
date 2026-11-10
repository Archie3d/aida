set(work "${WORKDIR}/task-services-semantics")
file(MAKE_DIRECTORY "${work}")
function(reject name source diagnostic)
    file(WRITE "${work}/${name}.adb" "${source}\n")
    execute_process(COMMAND "${ADAC}" -o "${work}/${name}.ssa" "${work}/${name}.adb"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
    string(FIND "${errors}" "${diagnostic}" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "${name}: expected ${diagnostic}, got ${status}\n${output}${errors}")
    endif()
endfunction()
reject(duplicate "procedure Root is task type T is pragma Priority (15); pragma Priority (14); end T;
task body T is begin null; end T; begin null; end Root;" "duplicate Priority pragma")
reject(duplicate_stack "procedure Root is task type T is pragma Storage_Size (1000); pragma Storage_Size (2000); end T;
task body T is begin null; end T; begin null; end Root;" "duplicate Storage_Size pragma")
reject(protected_stack "procedure Root is protected P is pragma Storage_Size (1000); procedure Go; end P;
protected body P is procedure Go is begin null; end Go; end P; begin null; end Root;" "not supported in this concurrent definition")
reject(real_stack "procedure Root is task type T is pragma Storage_Size (1.0); end T;
task body T is begin null; end T; begin null; end Root;" "integer type")
reject(main_priority "procedure Root is pragma Priority (31); begin null; end Root;" "static expression in System.Priority")
reject(dynamic_main "procedure Root is P : Integer := 15; pragma Priority (P); begin null; end Root;" "static expression in System.Priority")
reject(misplaced_stack "procedure Root is pragma Storage_Size (1000); begin null; end Root;" "immediate subprogram body")
reject(block_priority "procedure Root is begin declare pragma Priority (15); begin null; end; end Root;" "immediate subprogram body")
reject(package_priority "package Root is pragma Priority (15); end Root;" "immediate subprogram body")
reject(type_storage "procedure Root is task type T; task body T is begin null; end T;
N : Integer := T'Storage_Size; begin null; end Root;" "task object or its current instance")
reject(limited_attribute "with Ada.Task_Attributes; procedure Root is type T is limited null record;
X : T; package A is new Ada.Task_Attributes (T, X); begin null; end Root;" "limited")
reject(interrupt_policy "procedure Root is task T is pragma Interrupt_Priority; end T;
task body T is begin null; end T; begin null; end Root;" "unsupported or unknown pragma")

reject(task_body_priority "procedure Root is task T; task body T is pragma Priority (15);
begin null; end T; begin null; end Root;" "immediate subprogram body")

reject(priority_initialization "package Root is pragma Preelaborate;
function F return Integer;
protected type P is pragma Priority (F); procedure Go; end P;
pragma Preelaborable_Initialization (P); end Root;
package body Root is function F return Integer is begin return 15; end F;
protected body P is procedure Go is begin null; end Go; end P; end Root;"
"promised preelaborable initialization")
file(WRITE "${work}/task_access_storage.adb" "procedure Root is
    task type T; task body T is begin null; end T;
    type Ref is access all T; for Ref'Storage_Size use 0;
    X : Integer := Ref'Storage_Size;
begin if X /= 0 then raise Program_Error; end if; end Root;\n")
execute_process(COMMAND "${ADAC}" -o "${work}/task_access_storage.ssa" "${work}/task_access_storage.adb"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
if(NOT status STREQUAL "0" OR NOT errors STREQUAL "")
    message(FATAL_ERROR "access-to-task Storage_Size must retain access-pool semantics: ${status}\n${output}${errors}")
endif()
