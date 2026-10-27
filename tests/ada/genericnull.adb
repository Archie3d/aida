with Ada.Text_IO;
procedure GenericNull is
    generic
        type T is private;
        with procedure Hook (Object : in out T; Amount : Integer := 2) is null;
    package Views is
        procedure Invoke (Object : in out T);
        procedure Renamed (Object : in out T; Amount : Integer := 2) renames Hook;
    end Views;
    package body Views is
        procedure Invoke (Object : in out T) is
        begin
            Hook (Object => Object);
        end Invoke;
    end Views;
    procedure Hook (Object : in out Integer; Amount : Integer := 100) is
    begin
        raise Program_Error;
    end Hook;
    procedure Increment (Hidden : in out Integer; Amount : Integer := 100) is
    begin
        Hidden := Hidden + Amount;
    end Increment;
    package Default_View is new Views (Integer);
    package Explicit_View is new Views (Integer, Increment);
    generic
        with procedure Observe (Value : Integer) is null;
    procedure Evaluate (Count : in out Integer);
    procedure Evaluate (Count : in out Integer) is
        function Tick return Integer is
        begin
            Count := Count + 1;
            return 5;
        end Tick;
    begin
        Observe (Tick);
    end Evaluate;
    procedure Default_Evaluate is new Evaluate;
    subtype Triple is String (1 .. 3);
    generic
        with procedure Constrained (Value : Triple) is null;
    procedure Check_Bounds (Value : String);
    procedure Check_Bounds (Value : String) is
    begin
        Constrained (Value);
    end Check_Bounds;
    procedure Bounds is new Check_Bounds;
    generic
        with procedure Empty is null;
    procedure Invoke_Empty;
    procedure Invoke_Empty is
    begin
        Empty;
    end Invoke_Empty;
    procedure Default_Empty is new Invoke_Empty;
    procedure Check is
        Value : Integer := 3;
        Count : Integer := 0;
        Caught : Boolean := False;
        function Next_Value return Integer is
        begin
            Count := Count + 1;
            if Count = 3 then
                raise Constraint_Error;
            end if;
            return Count;
        end Next_Value;
        generic
            with procedure Action (Value : Integer := Next_Value) is null;
        procedure Invoke_Default;
        procedure Invoke_Default is
        begin
            Action;
        end Invoke_Default;
        procedure Default_Argument is new Invoke_Default;
        type Callback is access procedure (Object : in out Integer; Amount : Integer);
        Call : Callback := Explicit_View.Renamed'Access;
    begin
        if Count /= 0 then
            raise Program_Error;
        end if;
        Default_View.Invoke (Value);
        Default_View.Renamed (Value, 7);
        if Value /= 3 then
            raise Program_Error;
        end if;
        Explicit_View.Invoke (Value);
        Call (Value, 4);
        if Value /= 9 then
            raise Program_Error;
        end if;
        Default_Evaluate (Count);
        Default_Argument;
        begin
            Default_Argument;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        Default_Empty;
        Bounds ("abc");
        begin
            Bounds ("ab");
        exception
            when Constraint_Error => Caught := True;
        end;
        if Count /= 3 or not Caught then
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
    Ada.Text_IO.Put_Line ("null formal procedure defaults passed");
end GenericNull;
