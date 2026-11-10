set(work "${WORKDIR}/concurrent-interface-semantics")
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
reject(task_protected "procedure Root is type I is protected interface;
task type T is new I with end T; task body T is begin null; end T;
begin null; end Root;" "incompatible")
reject(protected_task "procedure Root is type I is task interface;
protected type P is new I with procedure Go; end P;
protected body P is procedure Go is begin null; end Go; end P;
begin null; end Root;" "incompatible")
reject(nonlimited "procedure Root is type I is interface;
task type T is new I with end T; task body T is begin null; end T;
begin null; end Root;" "nonlimited interface")
reject(conflicting "procedure Root is type I is task interface; type J is protected interface;
task type T is new I and J with end T; task body T is begin null; end T;
begin null; end Root;" "incompatible")
reject(noninterface "procedure Root is type R is tagged null record;
task type T is new R with end T; task body T is begin null; end T;
begin null; end Root;" "progenitor must name a specific interface")
reject(missing "procedure Root is package M is type I is synchronized interface;
procedure Go (Self : in out I) is abstract;
protected type P is new I with procedure Other; end P; end M;
package body M is protected body P is procedure Other is begin null; end Other; end P; end M;
begin null; end Root;" "must be overridden")
reject(readonly "procedure Root is package M is type I is protected interface;
procedure Go (Self : I) is abstract;
protected type P is new I with overriding procedure Go; end P; end M;
package body M is protected body P is procedure Go is begin null; end Go; end P; end M;
begin null; end Root;" "requires a variable controlling parameter")
reject(profile "procedure Root is package M is subtype Small is Integer range 1 .. 3;
type I is protected interface; procedure Go (Self : in out I; N : Small) is abstract;
protected type P is new I with overriding procedure Go (N : Integer); end P; end M;
package body M is protected body P is procedure Go (N : Integer) is begin null; end Go; end P; end M;
begin null; end Root;" "subtype conformant")
reject(not_override "procedure Root is package M is type I is protected interface;
procedure Go (Self : in out I) is abstract;
protected type P is new I with not overriding procedure Go; end P; end M;
package body M is protected body P is procedure Go is begin null; end Go; end P; end M;
begin null; end Root;" "marked not overriding")
reject(explicit_duplicate "procedure Root is package M is type I is protected interface;
procedure Go (Self : in out I) is abstract;
protected type P is new I with overriding procedure Go; end P;
overriding procedure Go (Self : in out P); end M;
package body M is protected body P is procedure Go is begin null; end Go; end P;
procedure Go (Self : in out P) is begin null; end Go; end M;
begin null; end Root;" "cannot also be implemented")
reject(private_full "procedure Root is package M is type I is synchronized interface;
type P is synchronized new I with private; private type P is limited new I with null record;
end M; begin null; end Root;" "requires a task or protected full view")
reject(copy "procedure Root is package M is type I is synchronized interface;
protected type P is new I with procedure Go; end P; end M;
package body M is protected body P is procedure Go is begin null; end Go; end P; end M;
A, B : M.P; begin A := B; end Root;" "cannot be assigned")
reject(generic_constraint "procedure Root is type I is synchronized interface;
task type T (N : Integer := 1) is new I with end T; task body T is begin null; end T;
generic type Actual is synchronized new I with private; package G is end G;
package Instance is new G (T); begin null; end Root;" "actual subtype does not match generic formal derived type")
