with Ada.Finalization;
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
        A : Boolean;
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
        A : Integer;
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
        A, B : Integer;
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
begin
    null;
end RecordRepresentationErrors;
