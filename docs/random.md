# Random number generation (Ada 2005 A.5.2)

Ada.Numerics.Float_Random and generic Ada.Numerics.Discrete_Random provide the
complete [A.5.2 profiles](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-A-5-2.html):
limited private Generator, private State, Random, all three Reset overloads,
Save, Image, Value and static Max_Image_Width. Float_Random additionally exposes
Uniformly_Distributed as Float range 0.0 .. 1.0. Public operations are ordinary
Ada subprograms, including renamings, named calls and subprogram Access.

## State, initialization and lifetime

Each generator contains four inline 64-bit words. Its private completion extends
Limited_Controlled, satisfying the required finalization property. The standard
scope, array, allocator and Unchecked_Deallocation paths manage finalization;
there is no separate heap-owned random state or generator registry. State is an
ordinary copyable record containing those words, with predefined equality.
Every discrete instance has distinct Generator and State types.

Implicit generator initialization and default State values use the SplitMix64
expansion of initiator zero. This state is fixed across executions and agrees
with Reset(Gen, 0). Integer Reset sign-extends Standard.Integer's 32-bit value to
64 bits and expands it with SplitMix64. Its first output word is a bijection of
the seed, so distinct initiators produce distinct states. The mixing function
rapidly separates nearby integer seeds; no disjoint-substream guarantee is made.
Save copies state without advancing the generator. Reset from State validates
and copies the saved words, preserving the exact subsequent sequence.

Time-dependent Reset reads the UTC microsecond clock and obtains an atomic
monotonically increasing seed ticket, choosing the clock value or the previous
ticket plus one. Signed timestamps are biased into unsigned order. Clock
rollback and calls within one clock tick therefore do not repeat tickets in one
process. The minimum required elapsed interval for distinct internal states is
zero, including concurrent calls, until ticket exhaustion (Storage_Error).
Separate processes do not share the counter and can obtain equal tickets;
singleton result subtypes necessarily have identical constant output streams.
No guarantee is made that individual draws from distinct states differ.

## Algorithm and ranges

The runtime uses xoshiro256** 1.0, with a nonzero 256-bit state and raw transition
period 2**256 - 1. The state transition and SplitMix64 expansion follow the
authors' public-domain implementations:
[xoshiro256**](https://prng.di.unimi.it/xoshiro256starstar.c) and
[SplitMix64](https://prng.di.unimi.it/splitmix64.c).
These are reproducible simulation generators, not cryptographic generators.

Float_Random takes the upper 24 output bits and multiplies by 2**(-24), producing
the uniformly spaced binary32 lattice in [0.0, 1.0). Zero can occur; one cannot.
The calculation is exact for the supported binary32 Float representation.

Discrete_Random samples logical positions, including enumeration positions
independent of representation codes. Signed and modular ranges, negative-only
ranges, singleton ranges, captured runtime bounds and both complete 64-bit
domains are supported. For a range of width N, words below (2**64 mod N) are
rejected before reducing modulo N. This removes modulo-reduction bias. Width
2**64 is handled separately without overflowing a signed count or dividing by
zero. Negative positions are reconstructed without out-of-range conversions.

The raw output transformation is a permutation of its 64-bit input word. The
full nonzero state orbit visits every possible input word; rejection sampling
therefore reaches every position in a supported discrete range in finitely many
calls, including the A.5.2 requirement for ranges of at most 2**15 values.
This is a period/algorithm argument, not a claim that every finite sample is
uniform. Annex G strict-mode performance/conformance is outside this increment.

A null result subtype raises Constraint_Error when the discrete instance is
elaborated, before Random is called. Bounds captured by a runtime subtype remain
fixed when the variables used to declare that subtype later change.

## State images and errors

Max_Image_Width is 74. Image returns `AIDA-RNG1:` followed by four 16-digit
uppercase hexadecimal words, most significant digit first within each word.
The word order is the generator state order; the text is independent of host
byte order. This version identifies the xoshiro256** representation above.

Value requires exactly 74 characters, the exact version prefix, hexadecimal
digits and at least one nonzero word. Payload hex digits can be uppercase or
lowercase. Whitespace, trailing characters, embedded nonhex characters, other
versions and the all-zero state raise Constraint_Error. String lower bounds are
irrelevant. Decoding stages all words before publishing them; invalid input
does not alter a generator or an existing destination state. Image and Reset
from State also reject an invalid all-zero representation.

For saved states, Value(Image(S)) = S. State images can be exchanged explicitly
between instances or the two families: the raw engine is common, while each
instance retains its own result range and Ada type identity. No implicit State
conversion is provided. Future incompatible engines must use a different format
version. Cross-version sequence compatibility is not promised.

## Concurrent use and validation

Release validation passes all 1,842 tests. All 11 focused ASan/UBSan checks and
the ThreadSanitizer runtime check pass. The original 145 ACATS backfill outcomes
remain unchanged.

Independent generators share no mutable state. Only time-dependent Reset uses a
global atomic ticket. Sharing one generator requires application synchronization
for Random, Reset and Save; the limited `in` parameter permits hidden state
mutation and does not make Random read-only. A protected procedure is one way to
serialize access. Image and Value operate only on their value arguments.

`randomlibrary` covers default and seeded reproducibility, all Reset forms,
Save/State equality/Image/Value replay, invalid images, rebased strings, integer
extremes, represented/restricted enumerations, nonbinary modular types, high-half
modular ranges, full signed/unsigned 64-bit domains, singleton and null ranges,
captured runtime bounds, independent instances and nested generics. Generator
arrays and heap cleanup, callable profiles, four independent workers and a
protected shared generator are checked in whole/separate/deterministic modes.
`randomlibraryerrors` checks limitedness, instance identity, discrete actuals and
exact profiles.

`runtime.random` checks fixed reference words and float bit patterns, a forced
rejection-sampling path, full-width boundaries, malformed images with unchanged
destinations, unaligned helper storage and a deterministic two-million-draw
coverage/frequency control over all 32,768 values. Four native workers test
independent sequences, distribution/range controls and 400 distinct concurrent
time-reset states. Statistical controls use fixed seeds and generous thresholds.

Unchanged ACATS CXA5011 and CXA5012 pass their complete REPORT scenarios. The
local Impdef supplies an actually invalid Non_State_String and a one-second
Delay_Per_Random_Test. Phase 9 asynchronous select also enables unchanged
CXA5013, now graded by its complete REPORT protocol. Its coverage scenario is
not replaced by an edited upstream test. The
[assessment manifest](../tests/acats/random/manifest.json) pins original paths,
SHA-256/Git-blob hashes, language eligibility, support settings and reviewed outcomes at revision `2785c30c1f8577d97ac7b3043882f6818781e9c5`.
