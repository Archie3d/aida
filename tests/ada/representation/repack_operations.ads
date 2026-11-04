with Ada.Streams;
package Repack_Operations is
    type Parent is record
        Left, Right : Integer;
    end record;
    function Make (Left, Right : Integer) return Parent;
    function Measure (Item : Parent) return Integer;
    procedure Update (Item : in out Parent);
    procedure Initialize (Item : out Parent);
    procedure Fail (Item : in out Parent);
    function Load (Stream : access Ada.Streams.Root_Stream_Type'Class) return Parent;
    procedure Read (Stream : access Ada.Streams.Root_Stream_Type'Class; Item : out Parent);
    for Parent'Input use Load;
    for Parent'Read use Read;

    type Swapped is new Parent;
    for Swapped use record
        Left at 4 range 0 .. 31;
        Right at 0 range 0 .. 31;
    end record;

    type Unaligned is new Swapped;
    for Unaligned use record
        Left  at 0 range 0 .. 31;
        Right at 5 range 0 .. 31;
    end record;

    type Choice (Flag : Boolean := False) is record
        case Flag is
            when False => X : Integer range -16 .. 15 := -4 + Boolean'Pos (Flag);
            when True => Y : Integer range -16 .. 15 := 7 + Boolean'Pos (Flag);
        end case;
    end record;
    procedure Switch (Item : in out Choice);
    type Compact is new Choice;
    for Compact use record
        Flag at 0 range 5 .. 5;
        X    at 0 range 0 .. 4;
        Y    at 0 range 0 .. 4;
    end record;
    subtype Count is Integer range 0 .. 15;
    type Counted (N : Count := 3) is record
        Marker : Character := 'Z';
        Value : Integer := N;
    end record;
    type Byte_Count is new Counted;
    for Byte_Count use record
        N      at 0 range 0 .. 7;
        Marker at 1 range 0 .. 7;
        Value  at 4 range 0 .. 31;
    end record;
end Repack_Operations;
