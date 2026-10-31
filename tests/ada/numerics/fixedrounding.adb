with Ada.Text_IO;
procedure Fixedrounding is
    type Fine is delta 0.03125 range -100.0 .. 100.0;
    type Coarse is delta 0.125 range -100.0 .. 100.0;
    type Wide is delta 1.0 range -10000000000000000.0 .. 10000000000000000.0;
    Big : Wide := 9007199254740993.0;
    A : Fine := 0.0625;
    B : Fine := -0.0625;
    C : Coarse := Coarse (A);
    D : Coarse := Coarse (B);
    E : Coarse := Coarse (Fine'(0.0625));
    F : Coarse := 16#1.0#E-1;
    Exact_Step : constant := 1.0 / 8.0;
    Power_Step : constant := 2.0 ** (-3);
    Power_Value : Coarse := 2.0 ** (-3);
    type Ratio is delta Exact_Step range -10.0 .. 10.0;
    Rounded_Float : constant Float := 0.062499999;
    Actual_Float : Float := Rounded_Float;
    Static_Float : Coarse := Coarse (Rounded_Float);
    Runtime_Float : Coarse := Coarse (Actual_Float);
    Whole_Expression : Coarse := 0.01 * 10.0;
    Converted_Expression : Coarse := Coarse (0.01 * 10.0);
    Product : Coarse;
    Left : Fine := 1.5;
    Right : Coarse := 2.0;
begin
    if C /= 0.125 or D /= -0.125 or E /= C or F /= C
        or Power_Step /= 0.125 or Power_Value /= 0.125
        or Ratio'Small /= 0.125 or Big /= 9007199254740993.0 then
        raise Program_Error;
    end if;
    if Whole_Expression /= 0.125 or Converted_Expression /= Whole_Expression then
        raise Program_Error;
    end if;
    if Static_Float /= Runtime_Float or Static_Float /= 0.125 then
        raise Program_Error;
    end if;
    Product := Left * Right;
    if Product /= 3.0 then raise Program_Error; end if;
    for I in -20 .. 20 loop
        declare
            Input : Fine := Fine (I) / 16;
            Converted : Coarse := Coarse (Input);
            Via_Float : Coarse := Coarse (Float (Input));
        begin
            if Converted /= Via_Float then raise Program_Error; end if;
        end;
    end loop;
    Ada.Text_IO.Put_Line ("fixed-point exact rational scales and symmetric rounding");
end Fixedrounding;
