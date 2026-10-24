file(MAKE_DIRECTORY "${WORKDIR}")
function(reject name source diagnostic)
    set(work "${WORKDIR}/${name}")
    file(MAKE_DIRECTORY "${work}" "${work}/units")
    file(WRITE "${work}/${name}.ads" "${source}")
    execute_process(COMMAND "${ADAC}" -I "${SOURCES}" -D "${work}/units" "${work}/${name}.ads"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 20)
    string(FIND "${errors}" "${diagnostic}" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "Incorrect context-view diagnostic for ${name}: ${status}\n${output}${errors}")
    endif()
endfunction()

reject(object "limited with Limited_B; package Bad is X : Limited_B.Node; end Bad;\n"
    "an object declaration requires a complete type view")
reject(array "limited with Limited_B; package Bad is type A is array (1 .. 2) of Limited_B.Node; end Bad;\n"
    "an array component requires a complete type view")
reject(component "limited with Limited_B; package Bad is type R is record X : Limited_B.Node; end record; end Bad;\n"
    "a record component requires a complete type view")
reject(derive "limited with Limited_B; package Bad is type R is new Limited_B.Node with null record; end Bad;\n"
    "type derivation requires a complete type view")
reject(result "limited with Limited_B; package Bad is function F return Limited_B.Node; end Bad;\n"
    "a function result requires a complete type view")
reject(untagged_parameter "limited with Context_Secret; package Bad is procedure P (X : Context_Secret.Data); end Bad;\n"
    "this subprogram parameter requires a complete type view")
reject(allocate "limited with Limited_B; package Bad is type P is access Limited_B.Node; X : P := new Limited_B.Node; end Bad;\n"
    "an allocator requires a complete type view")
reject(select "limited with Limited_B; package Bad is type P is access Limited_B.Node; X : P; N : Integer := X.Value; end Bad;\n"
    "component selection requires a complete type view")
reject(operation "limited with Limited_B; package Bad is type P is access Limited_B.Node; X : P; N : Integer := X.Read; end Bad;\n"
    "component selection requires a complete type view")
reject(attribute "limited with Limited_B; package Bad is N : Integer := Limited_B.Node'Size; end Bad;\n"
    "this attribute requires a complete type view")
reject(constraint "limited with Limited_B; package Bad is subtype T is Limited_B.Nested.Scalar range 1 .. 2; end Bad;\n"
    "an incomplete subtype declaration cannot add constraints")
reject(callback_result "limited with Limited_B; package Bad is type P is access function return Limited_B.Node; end Bad;\n"
    "an access-to-subprogram result requires a complete type view")
reject(callback_untagged "limited with Context_Secret; package Bad is type P is access procedure (X : Context_Secret.Data); end Bad;\n"
    "an access-to-subprogram parameter requires a complete type view")
reject(invisible_operation "limited with Limited_B; package Bad is N : Integer := Limited_B.Read; end Bad;\n"
    "'Read' is not declared in 'Limited_B'")
reject(use "limited with Limited_B; use Limited_B; package Bad is end Bad;\n"
    "a limited package view cannot be named in a use clause")
reject(use_type "limited with Limited_B; use type Limited_B.Node; package Bad is end Bad;\n"
    "a use type clause requires a complete type view")
reject(body "limited with Limited_B; procedure Bad is begin null; end Bad;\n"
    "a limited with clause is not allowed on a body")
reject(self "limited with Bad; package Bad is end Bad;\n"
    "a limited with clause cannot name its own library unit")
reject(overlap "limited with Limited_B; with Limited_B; package Bad is end Bad;\n"
    "limited and nonlimited with clauses for the same package cannot overlap")
reject(rename "limited with Limited_B; package Bad renames Limited_B;\n"
    "a limited with clause is not allowed on a body or library renaming")
reject(generic "limited with Ada.Unchecked_Deallocation; package Bad is end Bad;\n"
    "a limited with clause requires a nongeneric library package declaration")
reject(subprogram "limited with PrefixedCalls; package Bad is end Bad;\n"
    "a limited with clause requires a nongeneric library package declaration")
reject(private_visible "private with Context_Secret; package Bad is X : Context_Secret.Data; end Bad;\n"
    "'context_secret.data' is not declared")
reject(private_nested "private with Context_Secret; package Bad is package Nested is X : Context_Secret.Data; end Nested; end Bad;\n"
    "'context_secret.data' is not declared")
reject(private_use "private with Context_Secret; use Context_Secret; package Bad is end Bad;\n"
    "unknown package 'Context_Secret' in use clause")
reject(private_body_profile "private with Context_Secret; procedure Bad (X : Context_Secret.Data) is begin null; end Bad;\n"
    "'context_secret.data' is not declared")
reject(private_child "with Context_Impl.Child; package Bad is end Bad;\n"
    "private child unit is not accessible in this context clause")
reject(private_root "private package Bad is end Bad;\n"
    "a private library unit must be a child unit")
reject(tagged_completion "package Bad is type T is tagged; type T is null record; end Bad;\n"
    "a tagged incomplete type requires a tagged full definition")
reject(incomplete_object "package Bad is type T is tagged; X : T; type T is tagged null record; end Bad;\n"
    "an object declaration requires a complete type view")
reject(incomplete_parameter "package Bad is type T; procedure P (X : T); type T is null record; end Bad;\n"
    "this subprogram parameter requires a complete type view")
reject(visible_completion "package Bad is type T is tagged; private type T is tagged null record; end Bad;\n"
    "an incomplete declaration in the visible part requires a completion in the same visible part")
reject(missing_completion "package Bad is private type T; end Bad;\n"
    "an incomplete declaration in a private part has no completion")

# Full analysis of B must not turn a later limited view into its full view.
set(work "${WORKDIR}/order")
file(MAKE_DIRECTORY "${work}" "${work}/units")
file(WRITE "${work}/bad.ads" "limited with Limited_B; package Bad is X : Limited_B.Node; end Bad;\n")
foreach(reverse FALSE TRUE)
    set(inputs "${SOURCES}/limited_b.ads" "${work}/bad.ads")
    if(reverse)
        list(REVERSE inputs)
    endif()
    execute_process(COMMAND "${ADAC}" -I "${SOURCES}" -D "${work}/units" ${inputs}
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 20)
    string(FIND "${errors}" "an object declaration requires a complete type view" found)
    if(NOT status STREQUAL "1" OR found EQUAL -1)
        message(FATAL_ERROR "Limited views depend on source argument order: ${status}\n${output}${errors}")
    endif()
endforeach()

reject(discriminants "package Bad is type T (Kind : Integer); type T (Choice : Integer) is null record; end Bad;\n"
    "completion discriminants do not conform to the incomplete declaration")
reject(freeze "package Bad is type T is tagged; procedure P; end Bad; package body Bad is procedure P is begin null; end P; end Bad;\n"
    "an incomplete declaration in the visible part requires a completion in the same visible part")
reject(private_primitive "package Bad is private type T is tagged; procedure P (X : T); end Bad;\n"
    "a private incomplete type with primitive declarations must be completed in the private part")

reject(alias_stays_incomplete "with Limited_A; package Bad is X : Limited_A.Incomplete_B; end Bad;\n"
    "an object declaration requires a complete type view")
reject(access_stays_incomplete "with Limited_A; package Bad is X : Limited_A.B_Access; N : Integer := X.Value; end Bad;\n"
    "component selection requires a complete type view")
reject(renamed_view "limited with Limited_B; with Context_Aliases; package Bad is X : Context_Aliases.Whole.Node; end Bad;\n"
    "an object declaration requires a complete type view")
reject(renamed_nested_view "limited with Limited_B; with Context_Aliases; package Bad is X : Context_Aliases.Nested.Data; end Bad;\n"
    "an object declaration requires a complete type view")
reject(private_body "private procedure Context_Client.Bad is begin null; end Context_Client.Bad;\n"
    "private may precede a library declaration or renaming, not a body")
reject(anonymous_callback_result "limited with Limited_B; package Bad is F : access function return Limited_B.Node; end Bad;\n"
    "an access-to-subprogram result requires a complete type view")
reject(anonymous_callback_parameter "limited with Context_Secret; package Bad is F : access procedure (X : Context_Secret.Data); end Bad;\n"
    "an access-to-subprogram parameter requires a complete type view")
reject(generic_classwide "limited with Limited_B; package Bad is generic type T is private; package G is end G; package I is new G (Limited_B.Node'Class); end Bad;\n"
    "a generic actual type requires a complete type view")

reject(exported_limited_rename "with Limited_A; package Bad is X : Limited_A.Renamed_B.Node; end Bad;\n"
    "'limited_a.renamed_b.node' is not declared")

reject(access_discriminant_completion "package Bad is type T (Item : access Integer); type T (Item : access Character) is null record; end Bad;\n"
    "completion discriminants do not conform to the incomplete declaration")
