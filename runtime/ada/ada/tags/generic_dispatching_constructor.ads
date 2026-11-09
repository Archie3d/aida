generic
    type T (<>) is abstract tagged limited private;
    type Parameters (<>) is limited private;
    with function Constructor (Params : not null access Parameters)
        return T is abstract;
function Ada.Tags.Generic_Dispatching_Constructor
    (The_Tag : Tag; Params : not null access Parameters) return T'Class;
-- Intrinsic convention and tag-selected dispatch are supplied by the compiler.
pragma Preelaborate (Ada.Tags.Generic_Dispatching_Constructor);
