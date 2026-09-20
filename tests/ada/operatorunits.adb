with Ada.Text_IO; use Ada.Text_IO;
with OperatorLibrary; use OperatorLibrary;
procedure OperatorUnits is
    A : Box := Make (12);
    B : Box := Make (2);
    C : Box;
begin
    C := A + B;
    Put_Line (Integer'Image (Value (C)));
    C := OperatorLibrary."+" (Left => A, Right => B);
    Put_Line (Integer'Image (Value (C)));
    if A = B and not (A /= B) and not "/=" (A, B) then
        Put_Line ("private equality");
    end if;
end OperatorUnits;
