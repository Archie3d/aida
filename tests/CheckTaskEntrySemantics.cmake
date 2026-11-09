set(work "${WORKDIR}/task-entry-semantics")
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
reject(outside "procedure Root is begin accept Work; end Root;" "an accept statement must occur in a task body")
reject(wrong_profile "procedure Root is task S is entry Work (X : Integer); end S;
task body S is begin accept Work (Y : Integer); end S; begin null; end Root;"
    "accept profile must fully conform")
reject(wrong_mode "procedure Root is task S is entry Work (X : in out Integer); end S;
task body S is begin accept Work (X : Integer); end S; begin null; end Root;"
    "accept profile must fully conform")
reject(nested_subprogram "procedure Root is task S is entry Work; end S;
task body S is procedure P is begin accept Work; end P; begin null; end S; begin null; end Root;"
    "an accept statement must occur in a task body")
reject(nested_same "procedure Root is task S is entry Work; end S;
task body S is begin accept Work do accept Work; end Work; end S; begin null; end Root;"
    "cannot nest another accept for the same entry")
reject(access_parameter "procedure Root is task S is entry Work (X : access Integer); end S;
task body S is begin null; end S; begin null; end Root;" "task entries cannot have access parameters")
reject(missing_index "procedure Root is task S is entry Work (1 .. 2); end S;
task body S is begin accept Work; end S; begin null; end Root;" "accept profile must fully conform")
reject(extra_index "procedure Root is task S is entry Work; end S;
task body S is begin accept Work (1); end S; begin null; end Root;" "accept profile must fully conform")
reject(bad_index_type "procedure Root is task S is entry Work (Float); end S;
task body S is begin null; end S; begin null; end Root;" "entry family index subtype must be discrete")
reject(value_return "procedure Root is task S is entry Work; end S;
task body S is begin accept Work do return 3; end Work; end S; begin null; end Root;"
    "a procedure cannot return a value")
reject(entry_access "procedure Root is task S is entry Work; end S;
task body S is begin null; end S; type P is access procedure; A : P := S.Work'Access;
begin null; end Root;" "an entry cannot be designated by an access-to-subprogram type")
reject(renaming "procedure Root is task S is entry Work; end S;
task body S is begin null; end S; procedure P renames S.Work; begin null; end Root;"
    "task entry renamings are not yet supported")
reject(nested_count "procedure Root is task S is entry Work; end S;
task body S is procedure P is N : Integer := Work'Count; begin null; end P;
begin null; end S; begin null; end Root;" "Count requires an entry of the current")
reject(wrong_default "procedure Root is task S is entry Work (N : Integer := 1); end S;
task body S is begin accept Work (N : Integer := 2); end S; begin null; end Root;"
    "accept default must conform")
reject(type_prefix "procedure Root is task type S is entry Work; end S;
task body S is begin null; end S; begin S.Work; end Root;"
    "task object, not a type name")
