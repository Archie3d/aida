with Ada.Text_IO;
procedure ProtectedGenerics is
    generic
        initial : Integer;
    package Model is
        protected type Counter is
            procedure Increment;
            function Get return Integer;
        private
            value : Integer := initial;
        end Counter;
        object : Counter;
    end Model;
    package body Model is
        protected body Counter is
            procedure Increment is
            begin
                value := value + 1;
            end Increment;
            function Get return Integer is
            begin
                return value;
            end Get;
        end Counter;
    end Model;
    package First is new Model (7);
    package Second is new Model (9);
begin
    First.object.Increment;
    if First.object.Get /= 8 or Second.object.Get /= 9 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("protected generics passed");
end ProtectedGenerics;
