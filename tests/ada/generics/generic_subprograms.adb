with Ada.Text_IO; use Ada.Text_IO;
procedure Generic_Subprograms is
    generic
        type Element is private;
        type Index_Type is (<>);
        type Array_Type is array (Index_Type range <>) of Element;
        with function "<" (Left, Right : Element) return Boolean is <>;
    procedure Sort (Items : in out Array_Type);

    procedure Sort (Items : in out Array_Type) is
        Temp : Element;
    begin
        for I in Items'Range loop
            for J in I .. Items'Last loop
                if I < J and then Items (J) < Items (I) then
                    Temp := Items (I);
                    Items (I) := Items (J);
                    Items (J) := Temp;
                end if;
            end loop;
        end loop;
    end Sort;

    type Numbers is array (Integer range <>) of Integer;
    procedure Ascending is new Sort (Integer, Integer, Numbers);
    procedure Descending is new Sort (Integer, Integer, Numbers, ">");
    Bias : Integer := 0;
    function Compare (A, B : Integer) return Boolean is
    begin
        Bias := Bias + 1;
        return A > B;
    end Compare;
    function Compare (A, B : Float) return Boolean is
    begin
        return A < B;
    end Compare;
    procedure Custom is new Sort
        (Element => Integer, Index_Type => Integer, Array_Type => Numbers, "<" => Compare);
    A : Numbers (-2 .. 1) := (4, 1, 3, 2);
    Empty : Numbers (1 .. 0);

    generic
        type Value_Type is private;
        with function Transform (Item : Value_Type) return Value_Type;
        with procedure Update (Item : in out Value_Type);
    procedure Apply (Item : in out Value_Type);
    procedure Apply (Item : in out Value_Type) is
    begin
        Item := Transform (Item => Item);
        Update (Item => Item);
    end Apply;
    function Twice (X : Integer) return Integer is
    begin
        return X * 2;
    end Twice;
    procedure Increment (X : in out Integer) is
    begin
        X := X + Bias;
    end Increment;
    procedure Change is new Apply (Integer, Twice, Increment);
    Value : Integer := 5;

    generic
        with function Add (L : Integer; R : Integer := 3) return Integer is "+";
    function Defaulted return Integer;
    function Defaulted return Integer is
    begin
        return Add (L => 4);
    end Defaulted;
    function Seven is new Defaulted;

    package Comparisons is
        function "<" (L, R : Integer) return Boolean;
    end Comparisons;
    package body Comparisons is
        function "<" (L, R : Integer) return Boolean is
        begin
            return L > R;
        end "<";
    end Comparisons;
    procedure Qualified is new Sort (Integer, Integer, Numbers, Comparisons."<");

    type Letter_Index is (First, Middle, Last);
    type Letters is array (Letter_Index range <>) of Character;
    procedure Letter_Sort is new Sort (Character, Letter_Index, Letters);
    Text : Letters (First .. Last) := ('z', 'a', 'm');
    type Character_Indexed is array (Character range <>) of Integer;
    procedure Character_Sort is new Sort (Integer, Character, Character_Indexed);
    By_Character : Character_Indexed ('a' .. 'c') := (3, 1, 2);
begin
    Ascending (A);
    if A /= (1, 2, 3, 4) then
        raise Program_Error;
    end if;
    Ascending (Empty);
    Descending (A);
    if A /= (4, 3, 2, 1) then
        raise Program_Error;
    end if;
    Ascending (A);
    Custom (A);
    if A /= (4, 3, 2, 1) or else Bias = 0 then
        raise Program_Error;
    end if;
    Bias := 7;
    Change (Value);
    if Value /= 17 or else Seven /= 7 then
        raise Program_Error;
    end if;
    Ascending (A);
    Qualified (A);
    if A /= (4, 3, 2, 1) then
        raise Program_Error;
    end if;
    Letter_Sort (Text);
    if Text (First) /= 'a' or else Text (Last) /= 'z' then
        raise Program_Error;
    end if;
    Character_Sort (By_Character);
    if By_Character ('a') /= 1 or else By_Character ('c') /= 3 then
        raise Program_Error;
    end if;
    Put_Line ("generic sorting, operators, profiles, defaults, and captures");
end Generic_Subprograms;
