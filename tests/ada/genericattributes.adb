with Ada.Text_IO;
procedure GenericAttributes is
    subtype Small is Integer range 1 .. 3;
    generic
        type T is (<>);
        with function Next (Object : T) return T;
        with function Previous (Object : T) return T;
        with function Encode (Object : T) return String;
        with function Decode (Text : String) return T;
        with function Minimum (Left, Right : T) return T;
        with function Maximum (Left, Right : T) return T;
    package Views is
        procedure Check (Last : T);
    end Views;
    package body Views is
        procedure Check (Last : T) is
        begin
            if Previous (Next (Last)) /= Last or Decode (Encode (Last)) /= Last
                or Minimum (Last, Next (Last)) /= Last or Maximum (Last, Next (Last)) /= Next (Last) then
                raise Program_Error;
            end if;
        end Check;
    end Views;
    package Numbers is new Views (Small, Small'Succ, Small'Pred, Small'Image, Small'Value, Small'Min, Small'Max);
    subtype Letters is Character range 'A' .. 'C';
    package Characters is new Views (Letters, Letters'Succ, Letters'Pred, Letters'Image, Letters'Value, Letters'Min, Letters'Max);
    generic
        with function Next (Arg : Integer := 3) return Integer;
    function Advance return Integer;
    function Advance return Integer is
    begin
        return Next;
    end Advance;
    function Renamed (Arg : Integer) return Integer renames Integer'Succ;
    function Explicit_Default is new Advance (Integer'Succ);
    generic
        with function Renamed (Arg : Integer := 4) return Integer is <>;
    function Box_Advance return Integer;
    function Box_Advance return Integer is
    begin
        return Renamed;
    end Box_Advance;
    function Box_Default is new Box_Advance;
    package Model is
        subtype Number is Integer range 1 .. 3;
    end Model;
    generic
        with function Next (Model : Integer) return Integer is Model.Number'Succ;
    function Captured (Object : Integer) return Integer;
    function Captured (Object : Integer) return Integer is
    begin
        return Next (Object);
    end Captured;
    procedure Check is
        package Model is
            type Number is (Hidden, Replacement);
        end Model;
        function Bound_Default is new Captured;
    begin
        Numbers.Check (3);
        Characters.Check ('C');
        if Explicit_Default /= 4 or Box_Default /= 5 or Bound_Default (3) /= 4 then
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
    Ada.Text_IO.Put_Line ("attribute subprogram actuals passed");
end GenericAttributes;
