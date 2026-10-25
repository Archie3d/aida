with Ada.Text_IO; use Ada.Text_IO;
procedure Subunits is
    Bias : constant Integer := 3;
    State : Integer := 0;
    procedure Work (Value : in out Integer);
    function Read (Value : Integer := 2) return Integer is separate;
    package Nested is
        function Add (Value : Integer) return Integer;
    private
        Offset : constant Integer := 5;
    end Nested;
    package body Nested is separate;
    procedure Work (Value : in out Integer) is separate;

    generic
        type Item is range <>;
    package Generic_Package is
        function Next (Value : Item) return Item;
    end Generic_Package;
    package body Generic_Package is separate;
    package Numbers is new Generic_Package (Integer);
    package Other_Numbers is new Generic_Package (Integer);
    task type Worker;
    task body Worker is
        Value : Integer := 1;
    begin
        Work (Value);
        if Value /= 16 then
            raise Program_Error;
        end if;
    end Worker;
    Value : Integer := 1;
begin
    Work (Value);
    Put_Line (Integer'Image (Value));
    Put_Line (Integer'Image (Read));
    Put_Line (Integer'Image (State));
    Put_Line (Integer'Image (Numbers.Next (10) + Other_Numbers.Next (20)));
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
end Subunits;
