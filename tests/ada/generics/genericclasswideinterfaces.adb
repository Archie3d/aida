with Ada.Text_IO;
procedure GenericClasswideInterfaces is
    package Model is
        type First is interface;
        function Read (Object : First) return Integer is abstract;
        type Second is interface;
        procedure Ping (Object : in out Second) is abstract;
        type Root is abstract tagged null record;
        function Value (Object : Root) return Integer is abstract;
        function Pointer_Value (Object : access Root) return Integer is abstract;
        type Child is new Root and Second and First with record
            Count : Integer := 10;
        end record;
        overriding function Read (Object : Child) return Integer;
        overriding procedure Ping (Object : in out Child);
        overriding function Value (Object : Child) return Integer;
        overriding function Pointer_Value (Object : access Child) return Integer;
    end Model;
    package body Model is
        function Read (Object : Child) return Integer is
        begin
            return Object.Count;
        end Read;
        procedure Ping (Object : in out Child) is
        begin
            Object.Count := Object.Count + 1;
        end Ping;
        function Value (Object : Child) return Integer is
        begin
            return Object.Count + 10;
        end Value;
        function Pointer_Value (Object : access Child) return Integer is
        begin
            return Object.Count + 20;
        end Pointer_Value;
    end Model;
    generic
        type T (<>) is new Model.First and Model.Second with private;
    procedure Interface_Check (Object : in out T);
    procedure Interface_Check (Object : in out T) is
    begin
        Ping (Object);
        if Read (Object) /= 11 then
            raise Program_Error;
        end if;
    end Interface_Check;
    generic
        type T (<>) is new Model.Root with private;
    procedure Abstract_Check (Object : in out T);
    procedure Abstract_Check (Object : in out T) is
    begin
        if Value (Object) /= 21 or Pointer_Value (Object'Access) /= 31 then
            raise Program_Error;
        end if;
    end Abstract_Check;
    procedure Check_Interfaces is new Interface_Check (Model.Child'Class);
    procedure Check_Abstract is new Abstract_Check (Model.Root'Class);
    procedure Check is
        Object : aliased Model.Child := (Model.Root with Count => 10);
    begin
        Check_Interfaces (Object);
        Check_Abstract (Object);
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
    Ada.Text_IO.Put_Line ("interface class-wide generic actuals passed");
end GenericClasswideInterfaces;
