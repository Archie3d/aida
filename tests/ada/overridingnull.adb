with Ada.Text_IO;
procedure OverridingNull is
    procedure Run is
        procedure Early;
        type Callback is access procedure;
        Invoke : Callback := Early'Access;
        function Trigger return Integer is
        begin
            Early;
            Invoke.all;
            return 5;
        end Trigger;
        Value : Integer := Trigger;
        procedure Early is null;
        generic
            type Item is private;
        procedure Generic_Reset (Self : in out Item);
        procedure Generic_Reset (Self : in out Item) is
        begin
            null;
        end Generic_Reset;
        generic
            type Number is range <>;
        function Generic_Add (Left, Right : Number) return Number;
        function Generic_Add (Left, Right : Number) return Number is
        begin
            return Left + Right;
        end Generic_Add;
        generic
            type Item is private;
        function Equivalent (Left, Right : Item) return Boolean;
        function Equivalent (Left, Right : Item) return Boolean is
        begin
            return True;
        end Equivalent;
        package Equals is
            type Root is abstract tagged null record;
            function "=" (Left, Right : Root) return Boolean is abstract;
            type Child is new Root with null record;
            overriding function "=" is new Equivalent (Child);
        end Equals;
        package Numbers is
            type Number is range -100 .. 100;
            overriding function "+" is new Generic_Add (Number);
        end Numbers;
        package Model is
            type Forward is tagged;
            procedure Touch (Self : in out Forward) is null;
            type Forward is tagged null record;
            type Root is tagged record
                Value : Integer := 12;
            end record;
            not overriding procedure Keep (Self : in out Root) is null;
            procedure Reset (Self : in out Root) is null;
            procedure Replace (Self : in out Root) is null;
            type Child is new Root with null record;
            overriding procedure Keep (Self : in out Child) is null;
            overriding procedure Reset is new Generic_Reset (Child);
            overriding procedure Replace (Self : in out Child) renames Keep;
        end Model;
        Equal_Object : Equals.Child;
        Forward_Object : Model.Forward;
        Object : Model.Child;
        Wide : Model.Root'Class := Object;
        Calls : Natural := 0;
        function Argument return Integer is
        begin
            Calls := Calls + 1;
            return 8;
        end Argument;
        procedure Nothing (Left : in out Integer; Right : Integer := Argument) is null;
        procedure Completed (Left : in out Integer);
        procedure Completed (Left : in out Integer) is null;
        protected Guard is
            not overriding procedure Touch;
            not overriding entry Open;
        end Guard;
        protected body Guard is
            procedure Touch is
            begin
                null;
            end Touch;
            entry Open when True is
            begin
                null;
            end Open;
        end Guard;
    begin
        if Equals."/=" (Equal_Object, Equal_Object) then
            raise Program_Error;
        end if;
        Model.Touch (Forward_Object);
        Model.Keep (Wide);
        Model.Reset (Wide);
        Nothing (Value);
        Nothing (Value, Argument);
        Completed (Value);
        Guard.Touch;
        Guard.Open;
        if Calls /= 2 or else Value /= 5 or else Wide.Value /= 12
            or else Integer (Numbers."+" (2, 3)) /= 5 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("overriding/null worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("overriding and null procedures passed");
end OverridingNull;
