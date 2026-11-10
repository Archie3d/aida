set(work "${WORKDIR}/task-termination-semantics")
file(MAKE_DIRECTORY "${work}")
function(reject name source)
    file(WRITE "${work}/${name}.adb" "${source}\n")
    execute_process(COMMAND "${ADAC}" -o "${work}/${name}.ssa" "${work}/${name}.adb"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
    if(NOT status STREQUAL "1" OR NOT errors MATCHES "error:")
        message(FATAL_ERROR "${name}: expected a diagnosed rejection, got ${status}\n${output}${errors}")
    endif()
endfunction()
set(context "with Ada.Task_Termination; use Ada.Task_Termination;
with Ada.Task_Identification; use Ada.Task_Identification;
with Ada.Exceptions; use Ada.Exceptions;")
reject(ordinary "${context} procedure Root is
procedure Note (C : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence) is begin null; end Note;
H : Termination_Handler := Note'Access; begin null; end Root;")
reject(profile "${context} procedure Root is
protected P is procedure Note (C : Integer; T : Task_Id; X : Exception_Occurrence); end P;
protected body P is procedure Note (C : Integer; T : Task_Id; X : Exception_Occurrence) is begin null; end Note; end P;
H : Termination_Handler := P.Note'Access; begin null; end Root;")
reject(mode "${context} procedure Root is
protected P is procedure Note (C : in out Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence); end P;
protected body P is procedure Note (C : in out Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence) is begin null; end Note; end P;
H : Termination_Handler := P.Note'Access; begin null; end Root;")
