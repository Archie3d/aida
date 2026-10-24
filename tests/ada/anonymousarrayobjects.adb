with Ada.Text_IO; use Ada.Text_IO;
with AnonymousArrayData;
procedure AnonymousArrayObjects is
    type Cell is record
        Value : Integer;
    end record;
    A : array (1 .. 10) of access Cell := (others => new Cell);
    Empty : array (1 .. 2) of access Cell;
    Text : constant array (1 .. 3) of Character := "abc";
    Grid : array (1 .. 2, Boolean) of Integer := ((1, 2), (3, 4));
    Inferred : array (Positive range <>) of Integer := (4, 5, 6);
    Elements : array (1 .. 2) of aliased Integer := (7, 8);
    type Integer_Link is access all Integer;
    Pointer : Integer_Link := Elements (1)'Access;
    Calls : Integer := 0;
    function Bound return Integer is
    begin
        Calls := Calls + 1;
        return Calls + 1;
    end Bound;
    D, E : array (1 .. Bound) of Integer := (others => 0);
    Initializations : Integer := 0;
    function Next_Value return Integer is
    begin
        Initializations := Initializations + 1;
        return Initializations;
    end Next_Value;
    X, Y : array (1 .. 1) of Integer := (others => Next_Value);
    generic
        Size : Positive;
    package Buffers is
        function Sum return Integer;
    end Buffers;
    package body Buffers is
        function Sum return Integer is
            Data : array (1 .. Size) of Integer := (others => 1);
            Total : Integer := 0;
        begin
            for I in Data'Range loop
                Total := Total + Data (I);
            end loop;
            return Total;
        end Sum;
    end Buffers;
    package Three is new Buffers (3);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
begin
    for I in A'Range loop
        A (I).Value := I;
    end loop;
    for I in A'Range loop
        Check (A (I).Value = I);
    end loop;
    Check (A'Length = 10 and Empty (1) = null and Empty (2) = null);
    Check (Text (2) = 'b' and Grid (2, True) = 4);
    Check (Inferred'First = 1 and Inferred'Last = 3 and Inferred (3) = 6);
    Pointer.all := 9;
    Check (Elements (1) = 9);
    Check (Calls = 2 and D'Length = 2 and E'Length = 3);
    Check (Initializations = 2 and X (1) = 1 and Y (1) = 2);
    Check (Three.Sum = 3);
    Check (AnonymousArrayData.Items'Length = 2);
    AnonymousArrayData.Items (1).Value := 42;
    AnonymousArrayData.Items (2).Value := 43;
    Check (AnonymousArrayData.Items (1).Value = 42);
    Check (AnonymousArrayData.Items (2).Value = 43);
    Put_Line ("anonymous array objects passed");
end AnonymousArrayObjects;
