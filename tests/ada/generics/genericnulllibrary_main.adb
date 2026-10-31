with Ada.Text_IO;
with GenericNullLibrary;
procedure GenericNullLibrary_Main is
    package Default_View is new GenericNullLibrary (Integer);
    package Nested_Default is new Default_View.Inner;
    procedure Check is
        Count : Integer := 0;
        Value : Integer := 1;
        procedure Increment (Object : in out Integer; Amount : Integer := 100) is
        begin
            Count := Count + 1;
            Object := Object + Amount;
        end Increment;
        package Explicit_View is new GenericNullLibrary (Integer, 5, Increment);
        package Nested_Explicit is new Explicit_View.Inner;
        type Callback is access procedure (Object : in out Integer; Amount : Integer);
        Call : Callback := Explicit_View.Renamed'Access;
    begin
        Default_View.Invoke (Value);
        Default_View.Renamed (Value);
        Nested_Default.Invoke (Value);
        if Value /= 1 or Count /= 0 then
            raise Program_Error;
        end if;
        Explicit_View.Invoke (Value);
        Nested_Explicit.Invoke (Value);
        Call (Value, 2);
        if Value /= 12 or Count /= 3 then
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
    Ada.Text_IO.Put_Line ("library null procedure defaults passed");
end GenericNullLibrary_Main;
