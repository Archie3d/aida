with Ada.Text_IO; use Ada.Text_IO;
procedure ModularWideChecks is
    type Word is mod 2 ** 64;
    type Byte is mod 256;
    subtype Upper is Word range 2 ** 63 .. Word'Last;
    X : Word := Word'Last;
    S : Upper := X;
    I : Long_Integer := -1;
    F : Long_Float := -1.0;
    Zero : Word := 0;
    Count : Integer := 0;
begin
    begin
        I := Long_Integer (X);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        X := Word (I);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        X := Word (F);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        S := Zero;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        X := X / Zero;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        X := Word'Value ("18446744073709551616");
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        X := Word'Value ("-0");
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        I := Word'Pos (X);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        X := Word'Val (I);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    F := Long_Float (X);
    if Count /= 9 or F < 1.8E19 or S /= Word'Last then
        raise Program_Error;
    end if;
    F := 9223372036854775808.0;
    if Word (F) /= 2 ** 63 then
        raise Program_Error;
    end if;
    X := Word'Succ (X);
    if X /= 0 or Word'Pred (X) /= Word'Last then
        raise Program_Error;
    end if;
    Put_Line ("wide modular checks passed");
end ModularWideChecks;
