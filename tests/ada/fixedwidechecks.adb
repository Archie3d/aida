with Ada.Text_IO; use Ada.Text_IO;
procedure FixedWideChecks is
    type Fixed is delta 0.125 range -10.0 .. 10.0;
    Upper : Fixed := 2.0;
    subtype Window is Fixed range 1.0 .. Upper;
    W : Window := 1.5;
    N : Fixed := -1.5;
    One : Fixed := 1.0;
    type Byte is mod 256;
    B : Byte := 0;
    I : Integer;
    Count : Integer := 0;
    type Huge is delta 2.0 ** 62 range -2.0 ** 100 .. 2.0 ** 100;
    H : Huge := 2.0 ** 100;
    F : Float;
begin
    begin
        W := Window (0.5) + 1.0;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        W := Window'(0.5) + 1.0;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        declare
            subtype Bad is Window range 0.5 .. Upper;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := N * One;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        F := H * H;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    I := N * One;
    if Count /= 5 or W /= 1.5 or B /= 0 or I /= -2 then
        raise Program_Error;
    end if;
    Put_Line ("fixed conversions, dynamic constraints and result overflow checks");
end FixedWideChecks;
