with Ada.Text_IO;
procedure GenericContractReplay is
    function "+" (Left, Right : Integer) return Integer is
    begin
        return 99;
    end "+";
    generic
        type T is range <>;
    package Outer is
        generic
            X : T;
        function Sum return T;
    end Outer;
    package body Outer is
        function Sum return T is
        begin
            return X + X;
        end Sum;
    end Outer;
    package Integers is new Outer (Integer);
    function Add is new Integers.Sum (3);
    procedure Check is
    begin
        if Add /= 6 then raise Program_Error; end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("generic contract replay passed");
end GenericContractReplay;
