with Ada.Text_IO; use Ada.Text_IO;
procedure ProfileDefaults is
    type Pair is record
        First, Second : Integer;
    end record;
    package Values is
        Value : constant := 3;
        function Next (X : Integer) return Integer;
    end Values;
    package body Values is
        function Next (X : Integer) return Integer is begin return X + 1; end Next;
    end Values;
    use Values;
    procedure Check (A : Integer := 16#A#; B : Integer := Next (Value);
                     C : Pair := (First => 1, Second => 2); D : Integer := Integer'Last);
    procedure Check (A : Integer := 10; B : Integer := Values.Next (Values.Value);
                     C : Pair := (First => 1, Second => 2); D : Integer := Integer'Last) is
    begin
        if A /= 10 or B /= 4 or C.First /= 1 or C.Second /= 2 or D /= Integer'Last then
            raise Program_Error;
        end if;
    end Check;
    procedure Target (X : Integer) is
    begin
        if X /= 3 then raise Program_Error; end if;
    end Target;
    procedure Alias (X : Integer := Value);
    procedure Alias (X : Integer := Values.Value) renames Target;
    subtype Small is Integer range 1 .. 5;
    subtype Same is Integer range 1 .. 5;
    type Vector is array (Integer range <>) of Integer;
    subtype Two is Vector (1 .. 2);
    subtype Also_Two is Vector (1 .. 2);
    function Sum (X : Two) return Small;
    function Sum (X : Also_Two) return Same is
    begin
        return X (1) + X (2);
    end Sum;
begin
    Check;
    Alias;
    if Sum ((1, 2)) /= 3 then raise Program_Error; end if;
    Put_Line ("conforming defaults and subtypes checked");
end ProfileDefaults;
