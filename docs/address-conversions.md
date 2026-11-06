# System.Address_To_Access_Conversions (Ada 2005)

The source generic implements the profile in
[RM 13.7.2](https://www.adaic.org/resources/add_content/standards/05rm/html/RM-13-7-2.html):
Object is an indefinite limited-private formal; Object_Pointer is a distinct
access-all-Object type in each instance; To_Pointer and To_Address both use the
formal name Value. The package is Preelaborate and both functions have intrinsic
convention, including renamed views and generic subprogram actuals/defaults.
Subprogram Access on these intrinsic operations is diagnosed.

## Representation and supported actuals

The supported instances designate definite, fixed-layout object types: scalars,
private types with known completed layouts, plain and limited records, specific
tagged/controlled records, statically constrained arrays (including multiple
dimensions and packed Boolean arrays), and access-valued objects. Existing
object-access, representation and generic legality rules continue to apply.
Instances can be local, library-level, renamed, or nested in another generic;
Object_Pointer identities remain distinct, with ordinary compatible general
access conversions available explicitly.

Following RM 13.7.2(6), instantiation restrictions reject unconstrained or
runtime-bounded arrays, runtime-sized components, all discriminated records
(including constrained subtypes), class-wide actuals, and task/protected types
or components. An address alone cannot provide the runtime bounds, layouts or
constraint/view metadata used by those representations. Generic formal actuals
are checked again when an enclosing instance supplies a concrete type. These
restrictions are explicit diagnostics, not alternate pointer representations.

Supported access values are thin pointers. To_Pointer preserves null and the
address bits, with a runtime check against the designated type's native
alignment. An improperly aligned non-null address raises Program_Error. Null
always succeeds for a valid designated type. To_Address returns the same raw
pointer bits, mapping null to System.Null_Address without dereferencing designated storage.
The two conversions are inverse for supported aligned pointer representations;
To_Pointer(X'Address) equals X'Unchecked_Access for compatible live objects that
allow that attribute.

Alignment checking does not establish an object's type, size, initialization,
validity, accessibility, or lifetime. Other aligned addresses are accepted as
raw representations; accessing them still requires valid storage with the
proper Ada layout. Foreign scalar memory can be viewed while its original
owner keeps it alive. Controlled/tagged objects must already have valid Ada
initialization and tags; conversion does not construct them.

## Ownership and lifetime

Conversion neither allocates nor registers a designated object. It adds no
borrowed-view record, finalization node, accessibility extension or ownership
claim. The original stack scope, collection, master or foreign allocator keeps
responsibility for lifetime and cleanup. Keeping an Object_Pointer does not
prevent an aliased object's finalization or an owner's deallocation.

Object_Pointer remains an ordinary general access type: its own allocators and
Unchecked_Deallocation work normally, with the existing collection/master
rules. Declaring an instance can initialize its normal empty access collection;
conversion never inserts a borrowed object into it. Converting foreign or stack
storage does not make that storage eligible for Unchecked_Deallocation. A
pointer must not be dereferenced after its designated object's lifetime ends.

The compiler recognizes the instantiated To_Pointer profile and supplies native
alignment as a hidden runtime argument. To_Address uses an ordinary scalar C
entry point. The predefined profiles are validated before enabling intrinsic
lowering, so a malformed replacement declaration produces a diagnostic.
The helpers introduce no mutable runtime state; alignment failures use the
caller's existing task context.

## Validation and ACATS

Release validation passes all 1,818 tests. All 12 focused ASan/UBSan checks pass,
including malformed-input recovery and the compiler/runtime controls below.

`addressconversions` covers scalar/private/limited/controlled objects, aliased
components and elements, constrained strings/matrices/packed arrays, nulls,
address and Unchecked_Access identity, explicit and inherited access views,
renamings, named calls, generic actuals/defaults and nested/library instances.
Side-effecting arguments evaluate once. A misaligned address raises Program_Error
and a null dereference raises Constraint_Error. Foreign malloc storage survives
a temporary conversion instance and is freed by its C owner. Controlled scope
and heap controls verify exactly-once cleanup, no lifetime extension and no
finalization by a borrowed instance; instance-owned allocators/deallocation
remain operational. The environment task and four workers run independent
checks with a protected success count. Whole/separate compilation and
deterministic IR are checked.

`addressconversionerrors` checks instantiation restrictions, instance/type
mixing, incompatible profiles and intrinsic Access. The malformed-profile
control supplies a replacement generic missing To_Pointer's parameter; it must
reject normally rather than enable invalid lowering. The native
`runtime.address_conversions` test repeats boundary-address/alignment, local
memory, null and invalid-alignment cases in four C threads, checking per-task
exception isolation.

Unchanged CD70001 now passes its complete System/Storage_Elements scenario.
Unchanged CD72A01 and the official FD72A00 support execute their full address /
hex-string / pointer round trips, scalar and record dereferences and
Unchecked_Access/null controls. Impdef.Address_Value_IO is an explicit local
Modular_IO(Integer_Address) instance; neither the test nor official support is
edited. Both executable cases join the regular ACATS label.

CD72A02 is reviewed but remains a compilation rejection: its task entries and
select statements stop parsing before its remaining class-wide/protected/task
scenarios can be assessed. It is not graded as a conformance pass or as proof
of the instantiation restrictions; local controls cover those separately.
CD30002 retains the existing object-alignment/stride-padding blockers.
The original 145 backfill outcomes are unchanged. The extended
[assessment manifest](../tests/acats/storage_elements/manifest.json) pins original
source bytes, SHA-256/Git-blob hashes, language eligibility, support settings and
remaining rejection observations at ACATS revision
`2785c30c1f8577d97ac7b3043882f6818781e9c5`.
