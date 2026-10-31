with Ada.Text_IO;
procedure GenericLiterals is
    package Model is
        type Color is (Red, Green, Blue);
        type Other is (Red, Yellow);
        type Glyph is ('A', 'a', 'Z');
        for Glyph use ('A' => 10, 'a' => 20, 'Z' => 30);
        type Sub_Color is new Color range Green .. Blue;
        type Letter is new Character range 'A' .. 'Z';
    end Model;
    use Model;
    generic
        type T is private;
        with function Pick return T;
    package Views is
        function Value return T;
        function Renamed return T renames Pick;
    end Views;
    package body Views is
        function Value return T is
        begin
            return Pick;
        end Value;
    end Views;
    package Colors is new Views (Color, Red);
    package Other_Views is new Views (Other, Red);
    package Upper is new Views (Glyph, 'A');
    package Lower is new Views (Glyph, Model.'a');
    package Characters is new Views (Character, 'a');
    package Qualified_Characters is new Views (Character, Standard.'A');
    package Letters is new Views (Letter'Base, Model.'a');
    package Sub_Colors is new Views (Sub_Color'Base, Model.Red);
    package Booleans is new Views (Boolean, True);
    generic
        with function Red return Color is <>;
    function Default_Color return Color;
    function Default_Color return Color is
    begin
        return Red;
    end Default_Color;
    function Box_Default is new Default_Color;
    generic
        with function Pick return Glyph is 'a';
    function Default_Glyph return Glyph;
    function Default_Glyph return Glyph is
    begin
        return Pick;
    end Default_Glyph;
    function Named_Default is new Default_Glyph;
    procedure Check is
        type Derived is new Color;
        Position : Integer := 0;
    begin
        case Derived (Color'(Model.Green)) is
            when Derived (Color'(Model.Red)) => Position := 1;
            when Derived (Color'(Model.Green)) => Position := 2;
            when Derived (Color'(Model.Blue)) => Position := 3;
        end case;
        if Position /= 2 then
            raise Program_Error;
        end if;
        if Colors.Value /= Color'(Red) or Other_Views.Value /= Other'(Red)
            or Upper.Value /= Glyph'('A') or Lower.Value /= Glyph'('a')
            or Characters.Value /= 'a' or Qualified_Characters.Value /= 'A'
            or Letters.Value /= Letter'Base'('a') or Sub_Colors.Value /= Sub_Color'Base'Val (0)
            or not Booleans.Value or Box_Default /= Color'(Red) or Named_Default /= Glyph'('a') then
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
    Ada.Text_IO.Put_Line ("enumeration literal subprogram actuals passed");
end GenericLiterals;
