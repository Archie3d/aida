with Ada.Text_IO; use Ada.Text_IO;
with Ada.Text_IO.Decimal_IO;
procedure DecimalChecks is
    type Money is delta 0.01 digits 6;
    subtype Small is Money digits 3;
    package IO is new Ada.Text_IO.Decimal_IO (Small);
    A : Money := 12.0;
    Zero : Money := 0.0;
    X : Small := 1.25;
    Limit : Money := 12.0;
    F : Long_Float := 1.0E30;
    Caught : Integer := 0;
    Last : Positive;
    Text : String (1 .. 2);
    procedure Copy (Item : out Money) is
    begin
        Item := A;
    end Copy;
begin
    begin
        X := Small (A);
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        Copy (X);
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    if X /= 1.25 then raise Program_Error; end if;
    begin
        X := A / Zero;
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        X := Money (F);
        raise Program_Error;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        IO.Get ("12.00", X, Last);
        raise Program_Error;
    exception when Data_Error => Caught := Caught + 1;
    end;
    begin
        IO.Put (Text, X);
        raise Program_Error;
    exception when Layout_Error => Caught := Caught + 1;
    end;
    begin
        declare
            subtype Bad is Money digits 3 range -Limit .. Limit;
        begin
            raise Program_Error;
        end;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        declare
            subtype Bad is Money digits 3 range -12.0 .. 12.0;
        begin
            raise Program_Error;
        end;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        declare
            subtype Parent is Money range -1.0 .. 1.0;
            subtype Bad is Parent digits 3;
        begin
            raise Program_Error;
        end;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        declare
            subtype Bad is Money digits 3 range Limit .. -Limit;
        begin
            raise Program_Error;
        end;
    exception when Constraint_Error => Caught := Caught + 1;
    end;
    if Caught /= 10 or X /= 1.25 then raise Program_Error; end if;
    Put_Line ("decimal constraint, arithmetic, copy-back and text checks");
end DecimalChecks;
