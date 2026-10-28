with Ada.Text_IO;
with Return_Model; use Return_Model;
with GenericLimitedLibrary;
with GenericLimitedGlobal;
procedure GenericLimitedLibrary_Main is
    function Valid (Value : Cell) return Boolean is
    begin
        return Value.Value = 31 and Value.Self.all'Address = Value.Value'Address;
    end Valid;
    function Scalar (N : Integer) return Integer is
    begin
        return N;
    end Scalar;
    function Scalar_Valid (Value : Integer) return Boolean is
    begin
        return Value = 31;
    end Scalar_Valid;
    procedure Check is
        package Nonlimited is new GenericLimitedLibrary (Integer, Scalar, Scalar_Valid, 31);
        package Defaulted is new GenericLimitedLibrary (Cell, Forward, Valid);
        package Explicit is new GenericLimitedLibrary (Cell, Forward, Valid, Make (31));
        function Nested is new Defaulted.Test_Other;
    begin
        if not Defaulted.Test or not Explicit.Test or not Nested or not Nonlimited.Test then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    if not GenericLimitedGlobal.Instance.Test or not GenericLimitedGlobal.Array_Instance.Valid then
        raise Program_Error;
    end if;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("limited object library passed");
end GenericLimitedLibrary_Main;
