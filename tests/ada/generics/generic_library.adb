with Ada.Text_IO; use Ada.Text_IO;
with Generic_Choice;
procedure Generic_Library is
    function Minimum is new Generic_Choice (Integer);
    function Maximum is new Generic_Choice (Integer, ">");
    type Item is record
        Key : Integer;
    end record;
    function "<" (Left, Right : Item) return Boolean is
    begin
        return Left.Key < Right.Key;
    end "<";
    function First_Item is new Generic_Choice (Item);
    A : Item := (Key => 7);
    B : Item := (Key => 2);
    C : Item := First_Item (A, B);
begin
    if Minimum (3, 5) /= 3 or else Maximum (3, 5) /= 5 or else C.Key /= 2 then
        raise Program_Error;
    end if;
    Put_Line ("library generic functions and record comparison");
end Generic_Library;
