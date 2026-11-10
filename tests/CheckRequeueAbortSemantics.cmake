set(work "${WORKDIR}/requeue-abort-semantics")
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
reject(requeue_outside "procedure Root is begin requeue Missing; end Root;" "requeue requires an enclosing")
reject(requeue_procedure "procedure Root is protected P is entry E; procedure Proc; end P;
protected body P is entry E when True is begin requeue Proc; end E;
procedure Proc is begin null; end Proc; end P; begin null; end Root;" "requeue target must be")
reject(requeue_nested_body "procedure Root is protected P is entry E; end P;
protected body P is entry E when True is procedure Inner is begin requeue E; end Inner;
begin null; end E; end P; begin null; end Root;" "requeue requires an enclosing")
reject(requeue_wrong_mode "procedure Root is protected P is entry E (N : Integer); entry F (N : in out Integer); end P;
protected body P is entry E (N : Integer) when True is begin requeue F; end E;
entry F (N : in out Integer) when True is begin null; end F; end P;
begin null; end Root;" "requeue target must be")
reject(requeue_subtype "procedure Root is protected P is entry E (N : Integer); entry F (N : Positive); end P;
protected body P is entry E (N : Integer) when True is begin requeue F; end E;
entry F (N : Positive) when True is begin null; end F; end P;
begin null; end Root;" "profile must be subtype conformant")
reject(requeue_actuals "procedure Root is task T is entry E (1 .. 2) (N : Integer); end T;
task body T is begin accept E (1) (N : Integer) do requeue E (1) (2); end E; end T;
begin null; end Root;" "requeue has no actual parameter part")
reject(requeue_deep "procedure Root is task T is entry E; end T;
task body T is begin accept E do declare task Inner is entry F; end Inner;
task body Inner is begin accept F; end Inner; begin requeue Inner.F; end; end E; end T;
begin null; end Root;" "target object is too deeply nested")
reject(requeue_outer_formal "procedure Root is task type Target is entry E; end Target;
task body Target is begin accept E; end Target;
task T is entry A (Object : in out Target); entry B; end T;
task body T is begin accept A (Object : in out Target) do accept B do requeue Object.E; end B; end A; end T;
begin null; end Root;" "target object is too deeply nested")
reject(requeue_ambiguous "procedure Root is protected P is entry E (N : Integer); entry F; entry F (N : Integer); end P;
protected body P is entry E (N : Integer) when True is begin requeue F; end E;
entry F when True is begin null; end F; entry F (N : Integer) when True is begin null; end F; end P;
begin null; end Root;" "ambiguous requeue target entry")
reject(abort_scalar "procedure Root is N : Integer := 0; begin abort N; end Root;" "an abort statement requires a task object")
reject(abort_type "procedure Root is task type T; task body T is begin null; end T;
begin abort T; end Root;" "requires a task object, not a type name")
reject(abort_access "procedure Root is task type T; task body T is begin null; end T;
type Ref is access T; Object : Ref := new T; begin abort Object; end Root;" "an abort statement requires a task object")
