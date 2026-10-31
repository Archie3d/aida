with Ada.Text_IO;
procedure WindowsFloatParameters is
    X : Float := -2.5;
    Y : Long_Float := 3.25;
    Calls : Integer := 0;
    function Mixed (A, B, C, D : Integer; E : Float; F : Long_Float) return Long_Float is
    begin
        Calls := Calls + 1;
        return Long_Float (A + B + C + D) + Long_Float (E) + F;
    end Mixed;
    function Boundary (A, B : Integer; C : Float; D : Long_Float) return Long_Float is
    begin
        return Long_Float (A + B) + Long_Float (C) + D;
    end Boundary;
    function Signs (A, B, C, D : Integer; E : Float; F : Long_Float) return Boolean is
    begin
        return Float'Copy_Sign (1.0, E) = -1.0
            and Long_Float'Copy_Sign (1.0, F) = -1.0;
    end Signs;
begin
    if not Signs (0, 0, 0, 0, -0.0, -0.0) then
        raise Program_Error;
    end if;
    -- The static link consumes a slot too: C is in a register, D on the stack.
    if Mixed (1, 2, 3, 4, X, Y) /= 10.75
        or else Boundary (1, 2, X, Y) /= 3.75 or else Calls /= 1 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("Windows floating parameters ok");
end WindowsFloatParameters;
