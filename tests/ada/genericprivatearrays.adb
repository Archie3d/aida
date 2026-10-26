with Ada.Text_IO;
procedure GenericPrivateArrays is
    generic
        type Item is private;
        Initial : Item;
        Target : in out Item;
    package Values is
        function Copy return Item;
        procedure Restore;
    end Values;
    package body Values is
        function Copy return Item is
        begin
            return Initial;
        end Copy;
        procedure Restore is
        begin
            Target := Initial;
        end Restore;
    end Values;
    function Bound return Integer is
    begin
        return 3;
    end Bound;
    procedure Check is
        subtype Text is String (2 .. 4);
        Source : Text := "abc";
        Target : String (8 .. 10) := "xyz";
        package Strings is new Values (Text, Source, Target);
        type Matrix is array (Integer range <>, Integer range <>) of Integer;
        subtype Square is Matrix (1 .. 2, 4 .. 5);
        A : Square := ((1, 2), (3, 4));
        B : Square := (others => (others => 0));
        package Matrices is new Values (Square, A, B);
        subtype Dynamic is String (10 .. 9 + Bound);
        D : Dynamic := "old";
        E : Dynamic := "new";
        package Runtime_Strings is new Values (Dynamic, D, E);
    begin
        Source := "def";
        D := "bye";
        Strings.Restore;
        Matrices.Restore;
        Runtime_Strings.Restore;
        if Target /= "abc" or Strings.Copy /= "abc" or B /= A or E /= "old"
            or Runtime_Strings.Copy /= "old" then
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
    Ada.Text_IO.Put_Line ("generic private array actuals passed");
end GenericPrivateArrays;
