with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with System;
procedure RecordDependent is
    function Offset (First, Second : System.Address) return Long_Integer;
    pragma Import (C, Offset, "__test_size_offset");
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    subtype Length is Natural range 0 .. 6;
    type Buffer (N : Length := 2; Present : Boolean := True) is record
        Text : String (1 .. N) := (others => 'x');
        case Present is
            when True => Number : Integer := 123;
            when False => Alternate : String (1 .. N) := (others => 'y');
        end case;
    end record;
    for Buffer use record
        N at 16 range 0 .. 31;
        Present at 7 range 0 .. 7;
        Text at 8 range 0 .. 63;
        Number at 0 range 0 .. 31;
        Alternate at 0 range 0 .. 47;
    end record;
    type Matrix is array (Integer range <>, Integer range <>) of Character;
    type Tile (N : Length := 2) is record
        Cells : Matrix (1 .. N, 1 .. N) := (others => (others => 'm'));
    end record;
    for Tile use record
        N at 36 range 0 .. 31;
        Cells at 0 range 0 .. 287;
    end record;
    Grid : Tile;
    Other_Grid : Tile := (3, Cells => (others => (others => 'n')));
    type Derived is new Buffer;
    type Buffers is array (1 .. 2) of Buffer;
    type Link is access Buffer;
    procedure Free is new Ada.Unchecked_Deallocation (Buffer, Link);
    A : Buffer;
    B : Buffer := (4, False, Text => "test", Alternate => "abcd");
    C : Derived := (3, False, Text => "one", Alternate => "two");
    Values : Buffers;
    Heap : Link := new Buffer'(B);
    procedure Replace (Target : in out Buffer; Source : Buffer) is
    begin
        Target := Source;
    end Replace;
    function Make return Buffer is
    begin
        return (6, True, Text => "longer", Number => 77);
    end Make;
begin
    Check (Tile'Size = 320 and Grid.Cells (2, 2) = 'm');
    Grid := Other_Grid;
    Grid.Cells (3, 3) := 'z';
    Check (Grid.Cells'Length (1) = 3 and Grid.Cells (3, 3) = 'z' and Other_Grid.Cells (3, 3) = 'n');
    Check (Offset (Grid'Address, Grid.Cells'Address) = 0);
    Check (Buffer'Size = 160 and A.Text = "xx" and A.Number = 123);
    Check (A.Text'Position = 8 and A.Text'Last_Bit = 63 and A.Text'Size = 16);
    Check (Offset (A'Address, A.Text'Address) = 8);
    Check (Offset (B'Address, B.Alternate'Address) = 0 and B.Alternate = "abcd");
    Replace (A, B);
    A.Alternate (2) := 'z';
    Check (A.N = 4 and A.Text = "test" and A.Alternate = "azcd" and B.Alternate = "abcd");
    Replace (A, Make);
    Check (A.N = 6 and A.Text = "longer" and A.Number = 77);
    Values (1) := A;
    Values (2) := B;
    Check (Values (1).Text = "longer" and Values (2).Alternate = "abcd");
    Check (Offset (Values (1)'Address, Values (2)'Address) = 20);
    Check (C.Text = "one" and C.Alternate = "two");
    Check (Heap.Text = "test" and Heap.Alternate = "abcd");
    Free (Heap);
    A := (0, False, Text => "", Alternate => "");
    Check (A.Text'Length = 0 and A.Alternate'Length = 0);
    Put_Line ("inline discriminant-dependent representation passed");
end RecordDependent;
