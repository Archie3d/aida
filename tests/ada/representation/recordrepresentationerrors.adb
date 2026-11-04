with Ada.Finalization; with System;
procedure RecordRepresentationErrors is
    Runtime : Integer := 4;
    type Pair is record
        A, B : Integer;
    end record;
    for Pair use record
        A at 0 range 0 .. 31;
        B at 0 range 0 .. 31;
    end record;
    type Duplicate is record
        A : Integer;
    end record;
    for Duplicate use record
        A at 0 range 0 .. 31;
        A at 4 range 0 .. 31;
        Unknown at 8 range 0 .. 31;
    end record;
    type Non_Static is record
        A : Integer;
    end record;
    for Non_Static use record
        A at Runtime range 0 .. 31;
    end record;
    type Negative is record
        A, B, C : Integer;
    end record;
    for Negative use record
        A at -1 range 0 .. 31;
        B at 0 range -1 .. 30;
        C at 0 range 5 .. 2;
    end record;
    type Bits is record
        A : aliased Boolean;
    end record;
    for Bits use record
        A at 0 range 0 .. 0;
    end record;
    type Short is record
        A : Integer;
    end record;
    for Short use record
        A at 0 range 0 .. 7;
    end record;
    type Misaligned is record
        A : Float;
    end record;
    for Misaligned use record
        A at 1 range 0 .. 31;
    end record;
    type Overflow is record
        A : Integer;
    end record;
    for Overflow use record
        A at 9223372036854775807 range 8 .. 39;
    end record;
    type Twice is null record;
    for Twice use record end record;
    for Twice use record end record;
    type Late is null record;
    Object : Late;
    for Late use record end record;
    subtype Alias is Pair;
    for Alias use record end record;
    type Tagged_Record is tagged null record;
    for Tagged_Record use record end record;
    type Dynamic (N : Natural) is record
        Text : String (1 .. N);
    end record;
    for Dynamic use record
        Text at 8 range 0 .. 7;
    end record;
    type Captured is record
        Text : String (1 .. Runtime);
    end record;
    for Captured use record end record;
    type Parent is record
        A, B : aliased Integer;
    end record;
    type Child is new Parent;
    for Child use record
        A at 4 range 0 .. 31;
        B at 0 range 0 .. 31;
    end record;
    type Size_Conflict is record
        A : Integer;
    end record;
    for Size_Conflict'Size use 32;
    for Size_Conflict use record
        A at 8 range 0 .. 31;
    end record;
    type Mod_Conflict is null record;
    for Mod_Conflict use record
        at mod 3;
    end record;
    type Nested (Outer, Inner : Boolean) is record
        Common : Integer;
        case Outer is
            when True =>
                Parent_Field : Integer;
                case Inner is
                    when True => Leaf : Integer;
                    when False => Other_Leaf : Integer;
                end case;
            when False => Other : Integer;
        end case;
    end record;
    for Nested use record
        Common at 0 range 0 .. 31;
        Parent_Field at 0 range 0 .. 31;
        Leaf at 0 range 0 .. 31;
        Other_Leaf at 4 range 0 .. 31;
        Other at 0 range 0 .. 31;
    end record;
    Data : Parent;
    Plain : Integer;
    Bad_Prefix : Integer := Plain'Position;
    Bad_Arguments : Integer := Data.A'First_Bit (1);
    type Discontiguous (N : Natural := 2) is record
        Text : String (1 .. N);
    end record;
    Disjoint : Discontiguous;
    Bad_Storage : Integer := Disjoint.Text'Last_Bit;
    type Packed_Overlap is record
        A, B : Boolean;
    end record;
    for Packed_Overlap use record
        A at 0 range 3 .. 3;
        B at 0 range 3 .. 3;
    end record;
    type Packed is record
        A : Boolean;
    end record;
    for Packed use record
        A at 0 range 0 .. 0;
    end record;
    Packed_Object : Packed;
    Bad_Address : System.Address := Packed_Object.A'Address;
    type Packed_Discriminant (Flag : Boolean := False) is null record;
    for Packed_Discriminant use record
        Flag at 0 range 0 .. 0;
    end record;
    Discriminated : aliased constant Packed_Discriminant := (Flag => True);
    type Boolean_Pointer is access constant Boolean;
    Bad_Access : Boolean_Pointer := Discriminated.Flag'Access;
    type Late_Shared is range 0 .. 3;
    type Has_Packed is record
        Value : Late_Shared;
    end record;
    for Has_Packed use record
        Value at 0 range 0 .. 1;
    end record;
    pragma Atomic (Late_Shared);
    type Early_Shared is mod 4;
    pragma Volatile (Early_Shared);
    type Has_Shared is record
        Value : Early_Shared;
    end record;
    for Has_Shared use record
        Value at 0 range 0 .. 1;
    end record;
    subtype Dynamic_Bound is Integer range 0 .. Runtime;
    type Has_Dynamic_Bits is record
        Value : Dynamic_Bound;
    end record;
    for Has_Dynamic_Bits use record
        Value at 0 range 0 .. 3;
    end record;
    type Too_Wide is record
        Value : Long_Integer;
    end record;
    for Too_Wide use record
        Value at 0 range 0 .. 64;
    end record;
    package Indirect is
        type Plain is record
            Value : Integer;
        end record;
        procedure Observe (Anchor : Plain; Item : access Plain) is null;
        type Moved is new Plain;
        for Moved use record
            Value at 4 range 0 .. 31;
        end record;
    end Indirect;
    type Element is new Integer;
    type Aggregate_Part is array (1 .. 2) of Element;
    type Array_Parent is record
        Values : Aggregate_Part;
    end record;
    type Moved_Array is new Array_Parent;
    for Moved_Array use record
        Values at 4 range 0 .. 63;
    end record;
    pragma Atomic (Element);
begin
    null;
end RecordRepresentationErrors;
