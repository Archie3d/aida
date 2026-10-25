# Primitive operator visibility

Predefined operator profiles remain synthesized by the shared operator
resolver. They carry the declaring type's scope for visibility decisions
(Ada 2005 [RM 8.3](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-3.html)
and [8.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-8-4.html)).
An immediately visible predefined operator hides a
use-visible homograph. An inner or same-region explicit homograph hides or
overrides it. Operators with different profiles remain competing overloads;
conflicting overloadable operators imported from different regions remain
ambiguous. An implicit `/=` inherits its equality declaration's scope.

`use type` names a type through its subtype identity. It makes that type's
primitive operators available, including overridden operators declared in a
package that is not otherwise visible. Operators declared for a subtype in a
different package do not become primitive operations of the original type.
Private use clauses apply to bodies, private parts and private children, but
are excluded from the visible part of public children. Missing direct/use
visibility produces diagnostics instead of falling through to predefined
arithmetic or equality.

Package-qualified calls such as `P."="` resolve both explicit and implicit
operators declared in P. The prefix does not import operators through a
subtype or a use clause. Named actuals match predefined Left/Right (unary
Right) or the explicit declaration's own formal names. Chosen predefined
profiles stay bound through expression checking, including literal-only
modular arithmetic. Standard fixed multiplication/division retain their
existing deferred universal-fixed result context. Generic and renaming
wrappers retain an intrinsic already selected at their declaration site.

This repair covers operator expressions and calls. Callable views of implicit
operators, including qualified implicit operator renamings, generic actuals
and Access, retain their Phase 8 name/profile audit. Existing type, generic,
private-extension and static-capacity limits still apply.

`operatorvisibility` checks hiding, inner homographs, use type through a
subtype-only dependency, expanded/named calls, private types, spec/private/body
and private-child regions, and literal-only modular profiles. It runs whole/
per-unit/deterministic compilation and repeats from four tasks. The two
`operator*visibilityerrors` fixtures reject missing visibility, incorrect
qualification, private operators, homograph/formal-name conflicts and genuine
ambiguity, with private/public-child legal controls. The compiler passes
ASan/UBSan for these fixtures and unchanged C840001/C84008A/C84009A; all three
ACATS cases execute as PASS and join the strict suite. B840001 stays ungraded.
