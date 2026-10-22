# Modular types

The baseline is Ada 2005 RM [3.5.4](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5-4.html),
[3.5](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-3-5.html),
[12.5.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-12-5-2.html)
and [A.10.8](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-10-8.html).

All integer moduli from 1 through `2 ** 64` are supported. System exposes
`Max_Binary_Modulus = 2 ** 64` and `Max_Nonbinary_Modulus = 2 ** 64 - 1`.
Semantic bounds retain their mathematical values independently of the signed
machine cache. Runtime modular values use unsigned bit patterns in at most
64 bits; the helper ABI encodes modulus `2 ** 64` as zero. This encoding is
internal and does not change `'Modulus`.

Static and dynamic arithmetic share a helper with unsigned 128-bit
intermediates, including nonbinary multiplication and exponentiation. Logic,
unsigned comparisons, membership, loop endpoints and case choices preserve the
upper half of the unsigned range. Division, rem and mod by zero raise
Constraint_Error. Negative dynamic exponents also raise Constraint_Error.
Subtype checks still apply at assignment, call and conversion boundaries.

Ordinary conversions check the target range; they do not reduce modulo the
modulus. The Ada 2005 `S'Mod` attribute accepts any integer type, reduces even
negative arguments, and returns `S'Base`. Static arguments can use the existing
127-bit exact evaluator. `'Succ` and `'Pred` wrap at modular base endpoints.
`'Pos`, `'Val`, `'Image`, `'Value`, `'Width`, `'Min` and `'Max` handle full-width
unsigned values; `'Value` returns the base subtype and rejects a minus sign,
including `-0`. Conversion to signed types checks before interpreting unsigned
bits as signed values. Real conversions round away from zero at ties.

Generic `type T is mod <>` formals accept modular actuals; `range <>` formals
accept signed actuals. Modular_IO uses that contract and implements all six
Get/Put overloads, mutable default width/base, decimal and based output, string
Last positions, file token boundaries and explicit input widths. File input
uses call-owned growing storage under the existing per-file pin and lock;
there is no fixed token buffer. Invalid syntax/range raises Data_Error and a
short output string raises Layout_Error. Out actuals copy back on success.

Remaining restrictions:

- Array index subtypes must have signed 64-bit bounds, matching descriptor
  ABI v2. Use a narrower subtype when indexing by a wide modular type. Array
  elements may use the entire unsigned range.
- Variant choices and static explicit discriminants retain signed 64-bit
  storage. Upper-half variant choices are diagnosed.
- Types needing unsigned values above `2 ** 31 - 1` use 64-bit storage;
  an explicit 32-bit Size clause for such a type remains diagnosed. Size may
  select supported 8/16-bit compact layouts and 64-bit modular storage.
- Wide character attributes, general generic staticness rules, and the broader
  Text_IO line/page accounting audit remain with Phases 7–8.
- Hosts must support the project's existing 128-bit integer evaluation and
  two's-complement 64-bit storage requirements. Other-host execution remains
  part of the Phase 8 validation work.

`modularwide`, `modularwidechecks`, `modularwideerrors`, `modulario` and
`modulartasking` complement the original modular regressions. Whole/separate
execution and deterministic IR checks cover numeric code, library instantiation
and tasks. `runtime.modular` checks arithmetic identities, nonbinary products,
parsing, conversion failures and independent exception contexts on four threads;
it also runs under AddressSanitizer/UndefinedBehaviorSanitizer. The unchanged
upstream ACATS C354002 case covers modular scalar attributes, including the
maximum supported binary and nonbinary moduli. See the
[selected ACATS baseline](../tests/acats/README.md) for provenance.
