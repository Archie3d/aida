with Ada.Text_IO; use Ada.Text_IO;
procedure Generic_Fixed is
    generic
        type Real_Type is delta <>;
        Initial : Real_Type := 0.125;
    package Numbers is
        subtype Unit_Interval is Real_Type range 0.0 .. 1.0;
        subtype Positive is Real_Type range 0.0 .. Real_Type'Last;
        Step : constant Real_Type := Real_Type'Small;
        Requested : constant Real_Type := Real_Type (Real_Type'Delta);
        Value : Real_Type := Initial;
        Unit_Value : Unit_Interval := 0.5;
        function Calculate (X, Y : Real_Type) return Real_Type;
        function Round_Half return Real_Type;
    end Numbers;
    package body Numbers is
        function Calculate (X, Y : Real_Type) return Real_Type is
            Product : Real_Type := X * Y;
        begin
            return abs (-Product) + X / 2 - Y;
        end Calculate;
        function Round_Half return Real_Type is
        begin
            return Real_Type (0.0625);
        end Round_Half;
    end Numbers;

    generic
        type Real_Type is delta <>;
    function Large return Real_Type;
    function Large return Real_Type is
    begin
        return Real_Type (3000000000.0);
    end Large;

    generic
        type Source is delta <>;
        type Target is delta <>;
    function Convert (X : Source) return Target;
    function Convert (X : Source) return Target is
    begin
        return Target (X);
    end Convert;

    generic
        type Item is delta <>;
        with function Combine (L, R : Item) return Item is <>;
    function Apply (X : Item) return Item;
    function Apply (X : Item) return Item is
    begin
        return Combine (X, X);
    end Apply;

    type Coarse is delta 0.125 range -4000000000.0 .. 4000000000.0;
    type Fine is delta 0.01 range -100.0 .. 100.0;
    type Derived is new Coarse;
    subtype Narrow is Coarse range -1.0 .. 1.0;
    function Big is new Large (Coarse);
    package C is new Numbers (Coarse);
    package F is new Numbers (Real_Type => Fine, Initial => 0.25);
    package D is new Numbers (Derived);
    package N is new Numbers (Narrow);
    function To_Coarse is new Convert (Fine, Coarse);
    function Double is new Apply (Coarse, "+");
    generic
        type Item is delta <>;
    package Outer is
        package Inner is new Numbers (Item);
    end Outer;
    package Nested is new Outer (Fine);
    X : Narrow;
    Raised : Boolean := False;
begin
    if C.Step /= 0.125 or F.Step /= 0.0078125 or D.Step /= 0.125
        or C.Requested /= 0.125 or F.Requested /= 0.0078125
        or C.Value /= 0.125 or F.Value /= 0.25
        or C.Unit_Value /= 0.5 or F.Unit_Value /= 0.5
        or Nested.Inner.Step /= 0.0078125 then
        raise Program_Error;
    end if;
    if C.Calculate (1.5, 2.0) /= 1.75 or F.Calculate (1.5, 2.0) /= 1.75
        or C.Round_Half /= 0.125 or F.Round_Half /= 0.0625
        or To_Coarse (0.0625) /= 0.125 or Double (0.25) /= 0.5
        or Big /= 3000000000.0 then
        raise Program_Error;
    end if;
    begin
        X := N.Calculate (1.0, -1.0);
    exception
        when Constraint_Error => Raised := True;
    end;
    if not Raised then raise Program_Error; end if;
    Put_Line ("generic fixed: scales, contracts, conversions, operators, and range checks");
end Generic_Fixed;
