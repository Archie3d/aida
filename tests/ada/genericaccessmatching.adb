with Ada.Text_IO;
procedure GenericAccessMatching is
    subtype Small is Integer range 1 .. 3;
    type Narrow is access function (X : Small) return Small;
    function Next (X : Small) return Small is
    begin
        return X + 1;
    end Next;
    generic
        type Callback is access function (X : Integer) return Integer;
    function Apply (Call : Callback; X : Integer) return Integer;
    function Apply (Call : Callback; X : Integer) return Integer is
    begin
        return Call (X);
    end Apply;
    function Apply_Narrow is new Apply (Narrow);
    type Target is record
        Value : Integer;
    end record;
    type Pointer is access Target;
    generic
        type P is access Target;
    function Read (Value : P) return Integer;
    function Read (Value : P) return Integer is
    begin
        return Value.Value;
    end Read;
    generic
        type Element is private;
        type P is access Element;
    package Outer is
        generic
            type Q is access Element;
        function Copy (Value : Q) return P;
    end Outer;
    package body Outer is
        function Copy (Value : Q) return P is
        begin
            return new Element'(Value.all);
        end Copy;
    end Outer;
    procedure Check is
        -- This homonym must not change Read's designated subtype.
        type Target is new Integer;
        function Get is new Read (Pointer);
        package Nested is new Outer (GenericAccessMatching.Target, Pointer);
        function Clone is new Nested.Copy (Pointer);
        Source : Pointer := new GenericAccessMatching.Target'(Value => 19);
        Result : Pointer := Clone (Source);
    begin
        if Get (Result) /= 19 or Result = Source or Apply_Narrow (Next'Access, 2) /= 3 then
            raise Program_Error;
        end if;
        begin
            if Apply_Narrow (Next'Access, 3) = 0 then
                raise Program_Error;
            end if;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
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
    Ada.Text_IO.Put_Line ("generic access matching passed");
end GenericAccessMatching;
