# Case-selector overload resolution

A case selector expects any discrete type
([Ada 2005 RM 5.4(4)](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-5-4.html)).
The compiler discovers the selector's expression types without binding or
executing it, discards non-discrete and hidden private views, then binds the
expression using its unique discrete type. Calls, bare function names,
qualified names, operators and generic formal subprograms share this path.

Two different discrete result types remain ambiguous. Case choices do not
select a result type: they receive their context only after the selector is
resolved. A unique result type also cannot settle multiple matching argument
profiles; ordinary call resolution must still choose a unique interpretation.
The selected function's nominal result subtype is preserved for coverage
checks. Runtime bounds retain the existing base-range coverage behavior.
Only universal-integer selectors are normalized to Integer; universal-real
selectors remain non-discrete and are rejected.

`caseselector` and unchanged C540002 cover integer, modular, enumeration,
Boolean and character selectors against floating, fixed, string and record
alternatives. The local fixture checks qualified/named calls,
operators, defaulted result subtypes, runtime bounds, generic integer/modular/
discrete contexts, literal arithmetic and exactly-once selector/argument
execution. It runs whole/per-unit/deterministic compilation and repeats from
four tasks. `caseselectorerrors` rejects multiple discrete types, ambiguous
argument profiles, non-discrete overloads, real literals, hidden private views
and invalid generic contracts. Compiler ASan/UBSan checks cover both fixtures
and unchanged C540002, which executes as PASS in the strict ACATS suite.
Broader case-choice staticness and coverage audits retain their Phase 8 owner.
