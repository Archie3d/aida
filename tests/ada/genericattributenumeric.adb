with Ada.Text_IO;
procedure GenericAttributeNumeric is
    generic
        type T is private;
        with function Next (Object : T) return T;
        with function Previous (Object : T) return T;
        with function Encode (Object : T) return String;
        with function Decode (Text : String) return T;
        with function Minimum (Left, Right : T) return T;
        with function Maximum (Left, Right : T) return T;
    procedure Compare (One, Two : T);
    procedure Compare (One, Two : T) is
    begin
        if Previous (Next (One)) /= One or Decode (Encode (One)) /= One
            or Minimum (One, Two) /= One or Maximum (One, Two) /= Two then
            raise Program_Error;
        end if;
    end Compare;
    type Binary is digits 6;
    type Double is digits 15;
    type Fixed is delta 0.25 range -10.0 .. 10.0;
    type Decimal is delta 0.01 digits 6;
    type Unsigned is mod 2 ** 64;
    procedure Floats is new Compare (Binary, Binary'Succ, Binary'Pred, Binary'Image, Binary'Value, Binary'Min, Binary'Max);
    procedure Doubles is new Compare (Double, Double'Succ, Double'Pred, Double'Image, Double'Value, Double'Min, Double'Max);
    procedure Fixeds is new Compare (Fixed, Fixed'Succ, Fixed'Pred, Fixed'Image, Fixed'Value, Fixed'Min, Fixed'Max);
    procedure Decimals is new Compare (Decimal, Decimal'Succ, Decimal'Pred, Decimal'Image, Decimal'Value, Decimal'Min, Decimal'Max);
    procedure Modulars is new Compare (Unsigned, Unsigned'Succ, Unsigned'Pred, Unsigned'Image, Unsigned'Value, Unsigned'Min, Unsigned'Max);
    generic
        type T is digits <>;
        with function Minimum (Left, Right : T) return T is T'Min;
    function Min_Real (Left, Right : T) return T;
    function Min_Real (Left, Right : T) return T is
    begin
        return Minimum (Left, Right);
    end Min_Real;
    function Minimum is new Min_Real (Binary);
    procedure Check is
        Left : Binary := 1.0;
        Right : Binary := 2.0;
        Count : Integer := 0;
        function Argument return Integer is
        begin
            Count := Count + 1;
            return Count;
        end Argument;
    begin
        Floats (Left, Right);
        Doubles (1.0, 2.0);
        Fixeds (1.0, 2.0);
        Decimals (1.0, 2.0);
        Modulars (2 ** 63 + 1, 2 ** 63 + 2);
        if Minimum (Left, Right) /= Left or Integer'Max (Argument, Argument) /= 2
            or Count /= 2 or Integer'Min (3, 4) /= 3 or Binary'Max (1.0, 2.0) /= 2.0 then
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
    Ada.Text_IO.Put_Line ("numeric attribute subprogram actuals passed");
end GenericAttributeNumeric;
