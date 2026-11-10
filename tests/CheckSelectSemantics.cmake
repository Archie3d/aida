set(work "${WORKDIR}/select-semantics")
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
reject(ordinary_call "procedure Root is procedure P is begin null; end P;
begin select P; else null; end select; end Root;" "requires an entry call")
reject(select_outside "procedure Root is begin select accept Work; end select; end Root;"
    "a selective accept requires")
reject(terminate_delay "procedure Root is task S is entry Work; end S;
task body S is begin select accept Work; or terminate; or delay 0.0; end select; end S;
begin null; end Root;" "terminate, delay and else alternatives are mutually exclusive")
reject(guard_type "procedure Root is task S is entry Work; end S;
task body S is begin select when 1 => accept Work; else null; end select; end S;
begin null; end Root;" "a select guard requires a Boolean")
reject(conditional_guard "procedure Root is task S is entry Work; end S;
task body S is begin accept Work; end S;
begin select when True => S.Work; else null; end select; end Root;" "guards are allowed only")
reject(accept_abortable "procedure Root is task S is entry Work; end S;
task body S is begin select delay 0.0; then abort accept Work; end select; end S;
begin null; end Root;" "cannot occur in an abortable part")
reject(delay_forms "with Ada.Real_Time; procedure Root is task S is entry Work; end S;
task body S is begin select accept Work; or delay 0.0;
or delay until Ada.Real_Time.Clock; end select; end S;
begin null; end Root;" "delays must use the same relative or absolute")
reject(delay_types "with Ada.Real_Time; with Ada.Calendar;
procedure Root is task S is entry Work; end S;
task body S is begin select accept Work; or delay until Ada.Calendar.Clock;
or delay until Ada.Real_Time.Clock; end select; end S;
begin null; end Root;" "delays must use the same relative or absolute")
reject(terminate_tail "procedure Root is task S is entry Work; end S;
task body S is begin select accept Work; or terminate; null; end select; end S;
begin null; end Root;" "a terminate alternative cannot have following statements")
reject(terminate_else "procedure Root is task S is entry Work; end S;
task body S is begin select accept Work; or terminate; else null; end select; end S;
begin null; end Root;" "terminate, delay and else alternatives are mutually exclusive")
reject(async_multiple "procedure Root is begin select delay 0.0; or delay 0.0;
then abort null; end select; end Root;" "invalid timed, conditional or asynchronous")
reject(empty_abortable "procedure Root is begin select delay 0.0; then abort end select; end Root;"
    "an abortable part requires at least one statement")
reject(standalone_terminate "procedure Root is begin terminate; end Root;"
    "a terminate alternative is allowed only")
