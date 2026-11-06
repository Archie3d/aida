package General_Freezing is
    type Byte is range -64 .. 63;
    subtype Byte_View is Byte;
    Gap : constant := 1;
    for Byte'Size use 8;
    for Byte'Alignment use 1;

    type Ranged_Real is digits 6 range -1.0 .. 1.0;
    Real_Bits : constant := 32;
    for Ranged_Real'Size use Real_Bits;
    for Ranged_Real'Alignment use 4;

    type Color is (Red, Green, Blue);
    subtype Color_View is Color;
    for Color use (Red => -10, Green => 30, Blue => 250);
    for Color'Size use 16;

    type Vector is array (1 .. 2) of Integer;
    type Vector_Pointer is access Vector;
    for Vector'Size use 128;

    Shared_Value : Integer := 7;
    function Object_Default (Value : Integer := Shared_Value) return Integer;
    pragma Atomic (Shared_Value);

    type Owner;
    type Part (Ref : access Owner) is limited null record;
    type Owner is limited record
        Child : Part (Owner'Access);
    end record;
    for Owner use record
        Child at 0 range 0 .. Part'Size - 1;
    end record;

    type Source is record
        Left, Right : Integer;
    end record;
    function Make return Source;
    function Score (Item : Source) return Integer;
    type Pointer is access Source;
    function Selection (Value : Integer := Make.Right) return Integer;
    function Allocation (Value : Pointer := new Source'(Make)) return Integer;
    type Holder is record
        Value : Integer := Make.Right;
    end record;

    generic
        Value : Integer := Score (Make);
    package Defaults is
        Result : constant Integer := Value;
    end Defaults;
    package Explicit_Actual is new Defaults (9);

    for Source use record
        Left at 4 range 0 .. 31;
        Right at 0 range 0 .. 31;
    end record;
    for Source'Size use 128;

    procedure Check;
end General_Freezing;
