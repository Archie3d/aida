-- Shared sparse storage for both wide map families. Endpoints use 64 bits so
-- the one-past boundary of the complete 31-bit character domain is representable.
with Ada.Strings.Unbounded;
package Ada.Strings.Wide_Support is
    pragma Preelaborate;
    type Span is record
        Low, High : Long_Integer;
    end record;
    type Spans is array (Positive range <>) of Span;
    type Set_Kind is (Empty, Control, Graphic, Letter, Lower, Upper, Basic,
        Decimal_Digit, Hexadecimal_Digit, Alphanumeric, Special, ISO_646);
    type Set_Operation is (Union, Intersection, Exclusive_Or, Difference);
    type Set is record
        Prefix : Long_Integer := 0;
        Base : Set_Kind := Empty;
        Data : Unbounded.Unbounded_String;
    end record;
    Null_Set : constant Set := (Prefix => 0, Base => Empty, Data => <>);
    function Contains (Object : Set; Point : Long_Integer) return Boolean;
    function Ranges (Object : Set) return Spans;
    function Singleton (Low, High : Long_Integer) return Set;
    function Combine (Left, Right : Set; Operation : Set_Operation) return Set;
    type Mapping_Kind is (Unmapped, Lower_Case, Upper_Case, Basic_Case);
    type Mapping is record
        Base : Mapping_Kind := Unmapped;
        Data : Unbounded.Unbounded_String;
    end record;
    Identity : constant Mapping := (Base => Unmapped, Data => <>);
    function Value (Object : Mapping; Point : Long_Integer) return Long_Integer;
    procedure Add (Object : in out Mapping; From, Target : Long_Integer);
    function Domain (Object : Mapping) return Spans;
end Ada.Strings.Wide_Support;
