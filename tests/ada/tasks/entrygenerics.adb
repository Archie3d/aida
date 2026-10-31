with Ada.Text_IO;
procedure EntryGenerics is
    generic
        initial : Integer;
    package Model is
        protected type Counter is
            entry Add (amount : in out Integer);
            entry Add (amount : Boolean);
            function Get return Integer;
        private
            value : Integer := initial;
        end Counter;
        object : Counter;
    end Model;
    package body Model is
        protected body Counter is
            entry Add (amount : in out Integer) when value = initial is
            begin
                value := value + amount;
                amount := value;
            end Add;
            entry Add (amount : Boolean) when True is
            begin
                if amount then
                    value := value + 1;
                end if;
            end Add;
            function Get return Integer is
            begin
                return value;
            end Get;
        end Counter;
    end Model;
    package First is new Model (7);
    package Second is new Model (9);
    amount : Integer := 2;
begin
    First.object.Add (amount);
    if amount /= 9 then
        raise Program_Error;
    end if;
    amount := 3;
    Second.object.Add (amount);
    First.object.Add (True);
    if First.object.Get /= 10 or Second.object.Get /= 12 or amount /= 12 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("generic entries passed");
end EntryGenerics;
