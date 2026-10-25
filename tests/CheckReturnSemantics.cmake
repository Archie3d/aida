set(work "${WORKDIR}/return-semantics")
file(MAKE_DIRECTORY "${work}")
function(reject name source diagnostic)
    set(path "${work}/${name}.adb")
    file(WRITE "${path}" "${source}\n")
    execute_process(COMMAND "${ADAC}" -o "${work}/${name}.ssa" "${path}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 15)
    string(FIND "${errors}" "${diagnostic}" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "${name}: expected ${diagnostic}, got ${status}\n${output}${errors}")
    endif()
endfunction()
reject(scalar_constraint "function F return Integer is subtype Small is Integer range 1 .. 3;
begin return R : Small := 2; end F;"
    "extended return subtype must statically match")
reject(wrong_type "function F return Integer is begin return R : Float := 2.0; end F;"
    "extended return requires a compatible specific result subtype")
reject(array_constraint "procedure Root is subtype Three is String (1 .. 3);
function F return Three is begin return R : String (1 .. 2); end F; begin null; end Root;"
    "extended return subtype must statically match")
reject(indefinite "function F return String is begin return R : String; end F;"
    "an unconstrained array object needs an initializer or index constraint")
reject(procedure_return "procedure Root is begin return R : Integer := 1; end Root;"
    "extended return requires a compatible specific result subtype")
reject(nested_return "function F return Integer is begin return R : Integer := 1 do return 2; end return; end F;"
    "a return inside an extended return cannot have a value")
reject(out_of_scope "function F return Integer is begin return R : Integer := 1 do goto Done; end return;
<<Done>> return R; end F;"
    "'R' is not declared")
reject(constant "function F return Integer is begin return R : constant Integer := 1; end F;"
    "a return object cannot be declared constant in Ada 2005")
reject(named_access "procedure Root is type Pointer is access all Integer;
function F return Pointer is begin return R : access Integer := null; end F; begin null; end Root;"
    "extended return must use the same kind of subtype indication as the result")
reject(anonymous_access "procedure Root is type Pointer is access all Integer;
function F return access Integer is begin return R : Pointer := null; end F; begin null; end Root;"
    "extended return must use the same kind of subtype indication as the result")
set(limited_prefix "procedure Root is type Item is limited record X : Integer := 0; end record; A : Item;")
reject(limited_assignment "${limited_prefix} begin A := (X => 2); end Root;"
    "limited objects cannot be assigned")
reject(limited_copy "${limited_prefix} B : Item := A; begin null; end Root;"
    "limited objects cannot be copied")
reject(limited_return "${limited_prefix} function F return Item is begin return A; end F; begin null; end Root;"
    "a limited result must be built in place")
reject(limited_extended_copy "${limited_prefix} function F return Item is begin return R : Item := A; end F;
begin null; end Root;" "limited objects cannot be copied")
reject(limited_component "${limited_prefix} type Pair is record Value : Item; end record;
B : Pair := (Value => A); begin null; end Root;" "limited components cannot be copied")
reject(limited_array "${limited_prefix} type Items is array (1 .. 2) of Item;
B : Items := (others => A); begin null; end Root;" "limited components cannot be copied")
reject(limited_derived "${limited_prefix} type Derived is new Item; B, C : Derived;
begin B := C; end Root;" "limited objects cannot be assigned")
reject(limited_equality "${limited_prefix} begin if A = A then null; end if; end Root;"
    "a limited type has no predefined equality")
reject(tagged_components "${limited_prefix} type Tagged_Item is tagged record Value : Item; end record;
begin null; end Root;" "a tagged record with limited components must be explicitly limited")
reject(private_completion "package Root is type Item is private;
private type Item is limited record X : Integer; end record; end Root;"
    "a limited full view requires a limited private view")
# Concurrent result construction remains explicitly rejected until masters can
# adopt result tasks and defer activation to completion of the caller's object.
reject(task_result "procedure Root is task type Worker; task body Worker is begin null; end Worker;
function F return Worker is begin return R : Worker; end F; begin null; end Root;"
    "function results containing tasks are not yet supported")
reject(protected_result "procedure Root is protected type Guard is procedure P; end Guard;
protected body Guard is procedure P is begin null; end P; end Guard;
function F return Guard is begin return R : Guard; end F; begin null; end Root;"
    "function results containing protected objects are not yet supported")
