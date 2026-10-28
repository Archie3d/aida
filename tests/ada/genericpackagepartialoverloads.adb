with Ada.Text_IO;
procedure GenericPackagePartialOverloads is
    function Next (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Next;
    function Invert (Value : Boolean) return Boolean is
    begin
        return not Value;
    end Invert;
    generic
        with function F (Value : Integer) return Integer;
        with function F (Value : Boolean) return Boolean;
    package Model is
        function Number return Integer;
        function Flag return Boolean;
    end Model;
    package body Model is
        function Number return Integer is
        begin
            return F (1);
        end Number;
        function Flag return Boolean is
        begin
            return F (True);
        end Flag;
    end Model;
    package Data is new Model (Next, Invert);
    generic
        with package P is new Model (Next, others => <>);
    function Read return Boolean;
    function Read return Boolean is
    begin
        return P.Number = 2 and not P.F (True) and not P.Flag;
    end Read;
    function Read_Data is new Read (Data);
    generic
        with function "=" (Left, Right : Integer) return Boolean is <>;
    package Equalities is end Equalities;
    package Equality_Data is new Equalities;
    generic
        with package P is new Equalities ("=" => <>);
    function Different (Left, Right : Integer) return Boolean;
    function Different (Left, Right : Integer) return Boolean is
    begin
        return P."/=" (Left, Right);
    end Different;
    function Compare is new Different (Equality_Data);
    procedure Check is
    begin
        if not Read_Data or not Compare (1, 2) or Compare (1, 1) then
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
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("partial formal package overloads passed");
end GenericPackagePartialOverloads;
