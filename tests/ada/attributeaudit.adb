with Ada.Text_IO; use Ada.Text_IO;
procedure AttributeAudit is
    subtype Small is Integer range 1 .. 3;
    type Color is (Red, Green, Blue);
    subtype Middle is Color range Green .. Green;
    type Shade is new Middle;
    Limit : Integer := 3;
    subtype Dynamic is Integer range 1 .. Limit;
    I : Integer := 4;
    C : Color := Green;
    S : Shade := Green;
    Wide : Long_Integer := 4294967296;
    Negative : Integer := -7;
    type Modulo is mod 256;
    M : Modulo := 255;
    type Fixed is delta 0.25 range -10.0 .. 10.0;
    F : Fixed := 1.0;
    R : Float := 1.0;
    Long_R : Long_Float := 1.0;
    Invalid_Wide : Long_Integer := 4294967296;
    Invalid_Position : Integer := -1;
    Lowest_Real : Float := Float'Base'First;
    Highest_Fixed : Fixed'Base := Fixed'Base'Last;
    subtype Real_Window is Float range 2.0 .. 3.0;
    function Read (Value : Integer) return Integer is
    begin
        return Value;
    end Read;
begin
    Put_Line (Integer'Image (Small'Val (I)));
    Put_Line (Integer'Image (Dynamic'Val (I)));
    Put_Line (Color'Image (Middle'Succ (C)));
    Put_Line (Shade'Image (Shade'Succ (S)));
    Put_Line (Color'Image (Middle'Pred (C)));
    Put_Line (Color'Image (Middle'Val (0)));
    Put_Line (Long_Integer'Image (Long_Integer'Pos (Wide)));
    Put_Line (Long_Integer'Image (Integer'Pos (Negative)));
    Put_Line (Long_Integer'Image (Long_Integer'Val (Negative)));
    Put_Line (Boolean'Image (Fixed'Succ (F) = 1.25));
    Put_Line (Boolean'Image (Fixed'Pred (F) = 0.75));
    Put_Line (Boolean'Image (Float'Succ (R) > R));
    Put_Line (Boolean'Image (Float'Pred (R) < R));
    begin
        M := Modulo'Succ (M);
        Put_Line ("MISSED modular endpoint");
    exception
        when Constraint_Error => Put_Line ("modular endpoint");
    end;
    Wide := Long_Integer'Last;
    begin
        Wide := Long_Integer'Succ (Wide);
        Put_Line ("MISSED signed endpoint");
    exception
        when Constraint_Error => Put_Line ("signed endpoint");
    end;
    begin
        Put_Line (Integer'Image (Read (Integer'Val (Invalid_Wide))));
        Put_Line ("MISSED wide Val");
    exception
        when Constraint_Error => Put_Line ("wide Val");
    end;
    M := 0;
    begin
        M := Modulo'Pred (M);
        Put_Line ("MISSED modular predecessor");
    exception
        when Constraint_Error => Put_Line ("modular predecessor");
    end;
    begin
        C := Middle'Val (Invalid_Position);
        Put_Line ("MISSED static enumeration endpoint");
    exception
        when Constraint_Error => Put_Line ("static enumeration endpoint");
    end;
    R := Float'Base'Last;
    begin
        R := Float'Succ (R);
        Put_Line ("MISSED float endpoint");
    exception
        when Constraint_Error => Put_Line ("float endpoint");
    end;
    begin
        R := Float'Pred (Lowest_Real);
        Put_Line ("MISSED static float endpoint");
    exception
        when Constraint_Error => Put_Line ("static float endpoint");
    end;
    begin
        F := Fixed'Succ (Highest_Fixed);
        Put_Line ("MISSED fixed endpoint");
    exception
        when Constraint_Error => Put_Line ("fixed endpoint");
    end;
    Put_Line (Boolean'Image (Float'Succ (1.0) > 1.0));
    Put_Line (Boolean'Image (Long_Float'Pred (1.0) < 1.0));
    Put_Line (Boolean'Image (Long_Float'Succ (Long_R) > Long_R));
    Put_Line (Boolean'Image (Real_Window'First = 2.0 and Real_Window'Last = 3.0));
    Put_Line (Boolean'Image (Fixed'First = -10.0 and Fixed'Last = 10.0));
    Put_Line (Integer'Image (Small'Succ (Small'Last)));
    Wide := 4294967296;
    begin
        I := Long_Integer'Pos (Wide);
        Put_Line ("MISSED narrow Pos");
    exception
        when Constraint_Error => Put_Line ("narrow Pos");
    end;
end AttributeAudit;
