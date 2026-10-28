with Ada.Text_IO;
procedure GenericPackagePartialTypes is
    generic
        type Key is private;
        type Value is private;
        with function Matches (Object : Key) return Boolean;
    package Maps is
        subtype Key_Type is Key;
        subtype Value_Type is Value;
        function Check (Object : Key) return Boolean;
    end Maps;
    package body Maps is
        function Check (Object : Key) return Boolean is
        begin
            return Matches (Object);
        end Check;
    end Maps;
    function Positive (Object : Integer) return Boolean is
    begin
        return Object > 0;
    end Positive;
    function Upper (Object : Character) return Boolean is
    begin
        return Object = 'A';
    end Upper;
    package First is new Maps (Integer, Character, Positive);
    package Second is new Maps (Character, Boolean, Upper);
    generic
        with package Left is new Maps (others => <>);
        with package Right is new Maps (Key => Left.Value_Type, others => <>);
    package Join is
        function Check (Key : Left.Key_Type; Value : Right.Key_Type) return Boolean;
    end Join;
    package body Join is
        function Check (Key : Left.Key_Type; Value : Right.Key_Type) return Boolean is
        begin
            return Left.Check (Key) and Right.Check (Value);
        end Check;
    end Join;
    package Joined is new Join (First, Second);
    procedure Check is
    begin
        if not Joined.Check (1, 'A') or Joined.Check (0, 'A') then
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
    Ada.Text_IO.Put_Line ("dependent partial formal package types passed");
end GenericPackagePartialTypes;
