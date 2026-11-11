set(work "${WORKDIR}/concurrent-view-semantics")
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
reject(nondiscrete "procedure Root is task type T (N : Float); task body T is begin null; end T;
begin null; end Root;" "a discriminant has to be discrete")
reject(mixed_defaults "procedure Root is protected type P (A : Integer := 1; B : Integer) is procedure Go; end P;
protected body P is procedure Go is begin null; end Go; end P;
begin null; end Root;" "defaults must be provided for all discriminants")
reject(indefinite "procedure Root is task type T (N : Integer); task body T is begin null; end T; X : T;
begin null; end Root;" "has to fix its discriminants")
reject(write_task "procedure Root is task type T (N : Integer); task body T is begin N := 2; end T;
begin null; end Root;" "cannot be assigned to")
reject(write_protected "procedure Root is protected type P (N : Integer) is procedure Go; end P;
protected body P is procedure Go is begin N := 2; end Go; end P;
begin null; end Root;" "cannot be assigned to")
reject(write_external "procedure Root is protected type P (N : Integer) is procedure Go; end P;
protected body P is procedure Go is begin null; end Go; end P; X : P (1);
begin X.N := 2; end Root;" "discriminant")
reject(bad_family_bound "procedure Root is task type T (N : Integer) is entry E (1 .. N + 1); end T;
task body T is begin null; end T; begin null; end Root;" "discriminant must appear alone")
reject(single_task "procedure Root is task T (N : Integer); begin null; end Root;" "single task declaration cannot have discriminants")
reject(single_protected "procedure Root is protected P (N : Integer) is procedure Go; end P; begin null; end Root;" "only a protected type declaration can have discriminants")
reject(entry_access "procedure Root is task T is entry E; end T; task body T is begin null; end T;
procedure Renamed renames T.E; type Ref is access procedure; P : Ref := Renamed'Access;
begin null; end Root;" "entry cannot be designated")
reject(mode "procedure Root is task T is entry E (N : in out Integer); end T; task body T is begin null; end T;
procedure P (N : Integer) renames T.E; begin null; end Root;" "no subprogram matches the renaming profile")
reject(task_type "procedure Root is task type T is entry E; end T; task body T is begin null; end T;
procedure P renames T.E; begin null; end Root;" "requires a task object, not a type name")
reject(index_type "procedure Root is task T is entry E (1 .. 2); end T; task body T is begin null; end T;
procedure P renames T.E (True); begin null; end Root;" "no visible interpretation of 'True'")
reject(hidden_private "procedure Root is protected P is procedure Go; private procedure Secret; end P;
protected body P is procedure Go is begin null; end Go; procedure Secret is begin null; end Secret; end P;
procedure Alias renames P.Secret; begin null; end Root;" "no subprogram matches the renaming profile")
reject(protected_convention "procedure Root is procedure P is begin null; end P;
type Ref is access protected procedure; X : Ref := P'Access; begin null; end Root;" "no subprogram matches the designated access profile")
reject(ordinary_convention "procedure Root is protected P is procedure Go; end P;
protected body P is procedure Go is begin null; end Go; end P;
type Ref is access procedure; X : Ref := P.Go'Access; begin null; end Root;" "requires a protected subprogram access type")
reject(generic_entry_access "procedure Root is task T is entry E; end T; task body T is begin null; end T;
generic with procedure Op; procedure Check;
procedure Check is type Ref is access procedure; P : Ref := Op'Access; begin null; end Check;
procedure Instance is new Check (T.E); begin null; end Root;" "'Access of a generic formal subprogram is not allowed")
reject(callback_conversion "procedure Root is protected P is procedure Go; end P;
protected body P is procedure Go is begin null; end Go; end P;
type A is access protected procedure; type B is access protected procedure (N : Integer);
X : A := P.Go'Access; Y : B := B (X); begin null; end Root;" "this type conversion is not allowed")
