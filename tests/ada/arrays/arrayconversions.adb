with Ada.Text_IO; use Ada.Text_IO;
procedure ArrayConversions is
    type Vector is array (Integer range <>) of Integer;
    type Other is array (Positive range <>) of Integer;
    type Index is range -20 .. 20;
    type Indexed is array (Index range <>) of Integer;
    type Wide_Indexed is array (Long_Integer range <>) of Integer;
    subtype Shifted is Other (10 .. 12);
    A : Vector (-2 .. 0) := (1, 2, 3);
    B : Other := Shifted (A);
    C : Indexed := Indexed (A);
    Wide : Wide_Indexed := Wide_Indexed (A);
    Empty : Vector (8 .. 3) := (others => 0);
    E : Other := Other (Empty);
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    type Other_Matrix is array (Positive range <>, Positive range <>) of Integer;
    subtype Shifted_Matrix is Other_Matrix (4 .. 5, 8 .. 10);
    M : Matrix (-1 .. 0, -2 .. 0) := ((1, 2, 3), (4, 5, 6));
    N : Other_Matrix := Shifted_Matrix (M);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    procedure Change (V : in out Other) is
    begin
        Check (V'First = 10 and V'Last = 12);
        V (10) := 42;
    end Change;
    procedure Set_Value (V : out Other) is
    begin
        V := (7, 8, 9);
    end Set_Value;
    procedure Matrix_Change (V : in out Other_Matrix) is
    begin
        Check (V'First (1) = 4 and V'First (2) = 8);
        V (5, 10) := 99;
    end Matrix_Change;
    procedure Fixed (V : in out Shifted) is
    begin
        V (11) := 17;
    end Fixed;
    Bound_Calls : Integer := 0;
    function Bound return Integer is
    begin
        Bound_Calls := Bound_Calls + 1;
        return 1;
    end Bound;
    Source : Vector (1 .. 5) := (1, 2, 3, 4, 5);
    subtype Dynamic is Other (Bound .. Bound + 2);
    type Item is tagged record
        Count : Integer := 0;
    end record;
    type Items is array (1 .. 1) of Item;
    Shared : Items;
    procedure Mutate (X : in out Items) is
    begin
        X (1).Count := 17;
        Check (Shared (1).Count = 17);
    end Mutate;
    type Text is array (Positive range <>) of Character;
    Buffer : Text (1 .. 5) := (others => ' ');
    File : File_Type;
    Last : Natural;
    type Color is (Red, Green, Blue);
    subtype Two_Colors is Color range Red .. Green;
    type Colors is array (Color range <>) of Integer;
    type Two_Color_Values is array (Two_Colors range <>) of Integer;
    Color_Source : Colors (Red .. Green) := (11, 12);
    Color_Target : Two_Color_Values := Two_Color_Values (Color_Source);
    Color_Empty : Colors (Blue .. Green) := (others => 0);
    Converted_Empty : Two_Color_Values := Two_Color_Values (Color_Empty);
    subtype Component is Integer range 1 .. 3;
    type First_Array is array (Integer range <>) of Component;
    type Second_Array is array (Integer range <>) of Component;
    Original : First_Array (1 .. 2) := (1, 2);
    Converted : Second_Array := Second_Array (Original);
begin
    Check (B'First = 10 and B (12) = 3 and C'First = -2 and C (0) = 3);
    Check (Wide'First = -2 and Wide (0) = 3);
    Check (E'First = 8 and E'Last = 3 and E'Length = 0);
    Check (N'First (1) = 4 and N'First (2) = 8 and N (5, 10) = 6);
    Change (Shifted (A));
    Check (A (-2) = 42);
    Set_Value (Shifted (A));
    Check (A (-2) = 7 and A (0) = 9);
    Fixed (Other (Source (1 .. 3)));
    Check (Source (2) = 17);
    Set_Value (Dynamic (Source (Bound .. 3)));
    Check (Bound_Calls = 3 and Source (1) = 7 and Source (3) = 9);
    declare
        Nested_Value : Other := Other (Shifted (Source (1 .. 3)));
    begin
        Check (Nested_Value (12) = 9);
    end;
    Matrix_Change (Shifted_Matrix (M));
    Check (M (0, 0) = 99);
    A := Vector (B);
    Check (A (-2) = 1 and A (0) = 3);
    Check (Color_Target (Green) = 12 and Converted_Empty'Length = 0);
    Check (Converted (2) = 2);
    Mutate (Items (Shared));
    Create (File, Out_File, "arrayconversion.txt");
    Put_Line (File, "abc");
    Close (File);
    Open (File, In_File, "arrayconversion.txt");
    Get_Line (File, String (Buffer), Last);
    Close (File);
    Check (Last = 3 and Buffer (1) = 'a' and Buffer (3) = 'c');
    Put_Line ("array conversion values, bounds, sliding and writable views passed");
end ArrayConversions;
