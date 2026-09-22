with Ada.Text_IO; use Ada.Text_IO;

procedure ModularChecks is
    type Byte is mod 256;
    type Word is mod 2 ** 32;
    subtype Small is Byte range 1 .. 3;
    B : Byte := 255;
    Zero : Byte := 0;
    W : Word := Word'Last;
    S : Small := 1;
    N : Integer := -1;
    Wide : Long_Integer := 4294967296;
    F : Long_Float := 256.0;
    Count : Integer := 0;
begin
    begin
        B := B / Zero;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := B mod Zero;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := B rem Zero;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := B ** N;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := Byte (N);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        W := Word (Wide);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := Byte (F);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        N := Integer (W);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        S := S + 3;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := Byte'Succ (B);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := Byte'Pred (Zero);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := Byte (256);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        S := Small (4) - 1;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        S := Small'(4) - 1;
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        B := Small'(B);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    if Count /= 15 then
        Put_Line (Integer'Image (Count));
        raise Program_Error;
    end if;
    Put_Line ("modular checks passed");
end ModularChecks;
