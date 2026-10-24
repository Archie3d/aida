# Default discriminants and mutable records

The supported untagged record subset follows Ada 2005 RM 3.3.1, 3.7,
3.7.2 and 5.2. Discrete discriminants may have defaults, provided every
discriminant has one. Grouped declarations retain independent expressions;
defaults run once per discriminant per default-initialized object, with the
usual subtype checks. An explicit constraint or initializer supplies the
initial discriminants without evaluating their defaults.

A variable of an unconstrained subtype with defaults stays unconstrained even
when explicitly initialized. Whole-object assignment can change its
discriminants and active variants. The existing maximum record layout reserves
storage for every supported alternative, so changes need no allocation or
layout mutation. Selection and equality continue to inspect active variants.

An explicitly constrained object, a constant, an allocated object, or an object
of an indefinite subtype constrained by its initializer retains its original
discriminants. Assignment checks all discriminants before changing any target
storage; a mismatch raises Constraint_Error. Discriminants cannot be assigned
individually. Components require definite nominal subtypes; allocators of
indefinite record subtypes require a constraint or initializer.

`Object'Constrained` returns True for constants and values, and reports the
actual constraint for variables. In formals are constant views. Writable
unconstrained formals with defaulted discriminants receive a hidden 32-bit
constrainedness argument beside the object address. The callee saves it in its
lexical frame so nested calls and renamings retain the actual's constraint.
Direct calls, callbacks, generic instances, protected procedures/entries and
separate compilation use the same convention. Object layouts are unchanged;
rebuild callers and callees together (the incremental driver invalidates units
when the compiler changes).

Unconstrained record and array components with defaulted discriminants can
change their discriminants through component assignment. Renamings preserve
the renamed view. Boxed aggregate discriminants use declaration defaults;
variant selection still requires a static discriminant expression or static
boxed default, including `others => <>`.

Runtime explicit constraints and dependent component bounds now follow the
[runtime-discriminant contract](runtime-discriminants.md); controlled variant
components follow the [controlled-variant contract](controlled-variants.md).
Tagged discriminants, access discriminants, new derived
discriminant parts and general limited records retain their existing
restrictions. Named discriminant constraints landed with runtime constraints; discriminant
constraints on access subtypes remain unsupported. No broader representation or stream support is
claimed.

Evidence: `defaultdiscriminants`, `defaultdiscriminantstasking` and
`defaultdiscriminantslibrary` run whole/separate compilation and deterministic
IR checks, including four concurrent tasks, protected actions, callbacks,
generic instances, library globals, defaults and failed assignment preservation.
`defaultdiscriminanterrors` checks legality; `semantics.type_storage` checks
hidden-parameter selection through subtype and inherited-constraint chains.
Unchanged ACATS C37206A checks initialized unconstrained constants, and C37306A
checks assignment changing a defaulted variant discriminator.

Frontend UBSan checks cover the unit, legality and determinism tests. The
four-task scenario passes with an ASan/UBSan-instrumented C runtime and with
ThreadSanitizer; the controlled assignment scenario also passes with the
ASan/UBSan runtime. QBE-generated memory accesses remain uninstrumented.
