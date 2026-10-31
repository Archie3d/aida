procedure StaticArithmeticErrors is
    Too_Positive : Long_Integer := 9223372036854775808;
    Too_Negative : Long_Integer := -9223372036854775809;
    Too_Wide : Integer := 2147483647 + 1;
    Divide_Zero : Integer := 1 / 0;
    Mod_Zero : Integer := 1 mod 0;
    Rem_Zero : Integer := 1 rem 0;
    Negative_Power : Integer := 2 ** (-1);
    Capacity : Integer := (2 ** 126 * 2) / 2;
    Real_Capacity : Float := (1.0E38 * 10.0) / 10.0;
    Real_Zero : Float := 1.0 / 0.0;
    Exact_Literal : Float := 1.0E999999;
    Huge_Unused : constant := 2 ** 127;
    Eager : Boolean := False and 1 / 0 = 0;
    subtype Small is Integer range 1 .. 3;
    type Derived is new Small;
    Bad_Conversion : Small := Small (4);
    Bad_Qualification : Small := Small'(4);
    Bad_Derived : Derived := Derived (9);
    Bad_Wide_Conversion : Integer := Integer (2 ** 100);
    Bad_Attribute : Long_Integer := Long_Integer'Succ (Long_Integer'Last);
    Bad_Val : Boolean := Boolean'Val (2);
    Absolute : Long_Integer := abs (-9223372036854775808);
    Quotient : Long_Integer := (-9223372036854775808) / (-1);
    Named : constant := 2 ** 100;
    Bad_Named : Long_Integer := Named;
    Dynamic : Boolean := False;
    Bad_Mixed : Boolean := False and then 1 / 0 = Integer (Boolean'Pos (Dynamic));
    type Fixed is delta 0.5 range -10.0 .. 10.0;
    Bad_Fixed : Fixed := Fixed (11.0);
    Bad_Real_Conversion : Integer := Integer (1.0E30);
    type Whole is delta 1.0 range -9223372036854775808.0 .. 9223372036854775807.0;
    Fixed_Capacity : Whole := (Whole'Last + 1.0) - 1.0;
    Bad_Pos : Long_Integer := Long_Integer'Pos (Named);
    Qualified_Failure : Integer := Integer'(Long_Integer'Pos (Named));
begin
    case Named is
        when others => null;
    end case;
    for I in Named .. Named loop
        null;
    end loop;
end StaticArithmeticErrors;
