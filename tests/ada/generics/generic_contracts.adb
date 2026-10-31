with Ada.Text_IO; use Ada.Text_IO;
procedure Generic_Contracts is
    function External (X : Integer) return Integer is
    begin
        return X + 10;
    end External;

    generic
        type Left_Type is private;
        type Right_Type is private;
        with function Pick (X : Left_Type) return Integer;
        with function Pick (X : Right_Type) return Integer;
    function Choose (L : Left_Type; R : Right_Type) return Integer;
    function Choose (L : Left_Type; R : Right_Type) return Integer is
    begin
        return Pick (L) + Pick (R);
    end Choose;

    function First (X : Integer) return Integer is
    begin
        return 1;
    end First;
    function Second (X : Integer) return Integer is
    begin
        return 20;
    end Second;
    function Combined is new Choose (Integer, Integer, First, Second);

    generic
        type Item is range <>;
    function Lexical (X : Item) return Integer;
    function Lexical (X : Item) return Integer is
    begin
        return External (Integer (X));
    end Lexical;

    generic
        type Item is private;
        with function "=" (L, R : Item) return Boolean;
    function Different (L, R : Item) return Boolean;
    function Different (L, R : Item) return Boolean is
    begin
        return L /= R;
    end Different;
    function Equal (L, R : Integer) return Boolean is
    begin
        return L mod 10 = R mod 10;
    end Equal;
    function Unequal is new Different (Integer, Equal);

    generic
        type L_Type is private;
        type R_Type is private;
    package Overloaded is
        function Select_Value (X : L_Type) return Integer;
        function Select_Value (X : R_Type) return Integer;
        function Both (L : L_Type; R : R_Type) return Integer;
    end Overloaded;
    package body Overloaded is
        function Select_Value (X : R_Type) return Integer is
        begin
            return 20;
        end Select_Value;
        function Select_Value (X : L_Type) return Integer is
        begin
            return 1;
        end Select_Value;
        function Both (L : L_Type; R : R_Type) return Integer is
        begin
            return Select_Value (L) * 100 + Select_Value (R);
        end Both;
    end Overloaded;
    package Same_Types is new Overloaded (Integer, Integer);

    generic
        Initial : Integer;
    package Cell is
        Value : Integer := Initial;
    end Cell;
    generic
        type Item is private;
    package Pair is
        package A is new Cell (1);
        package B is new Cell (2);
        function Read_A return Integer;
    end Pair;
    package body Pair is
        function Read_A return Integer is
        begin
            return A.Value;
        end Read_A;
    end Pair;
    package Cells is new Pair (Integer);

    generic
        type Item is range <>;
    function Recursive (X : Item) return Item;
    function Recursive (X : Item) return Item is
    begin
        if X = 0 then
            return 0;
        end if;
        return 1 + Recursive (X - 1);
    end Recursive;
    function Count is new Recursive (Integer);
begin
    if Combined (0, 0) /= 21 or else Unequal (1, 11) or else Count (4) /= 4
        or else Same_Types.Both (0, 0) /= 120 or else Cells.Read_A /= 1 then
        raise Program_Error;
    end if;
    declare
        function External (X : Integer) return Integer is
        begin
            return 99;
        end External;
        function Bound is new Lexical (Integer);
    begin
        if Bound (2) /= 12 then
            raise Program_Error;
        end if;
    end;
    Put_Line ("generic contracts preserve names, overloads, equality, and recursion");
end Generic_Contracts;
