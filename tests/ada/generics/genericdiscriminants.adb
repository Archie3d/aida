with Ada.Text_IO;
procedure GenericDiscriminants is
    type Item (Length : Integer) is record
        D : Integer;
    end record;
    type Defaulted (Length : Integer := 3) is record
        D : Integer;
    end record;
    generic
        type T (D : Integer) is private;
    package View is
        subtype Three is T (D => 3);
        subtype Four is T (D => 4);
        function Read (X : T) return Integer;
        function Matches (X : T) return Boolean;
        function Definite return Boolean;
        generic
        function Nested (X : T) return Integer;
    end View;
    package body View is
        function Read (X : T) return Integer is
        begin
            return X.D;
        end Read;
        function Matches (X : T) return Boolean is
        begin
            return X in Three and then X not in Four;
        end Matches;
        function Definite return Boolean is
        begin
            return T'Definite;
        end Definite;
        function Nested (X : T) return Integer is
            subtype Expected is T (D => 3);
        begin
            if X not in Expected then
                raise Program_Error;
            end if;
            if T'Definite then
                return X.D + 10;
            end if;
            return X.D;
        end Nested;
    end View;
    package Plain is new View (Item);
    package Defaults is new View (Defaulted);
    function Plain_Nested is new Plain.Nested;
    function Default_Nested is new Defaults.Nested;
    procedure Check is
        X : Item := (3, 99);
        Y : Defaulted := (3, 88);
    begin
        if Plain.Read (X) /= 3 or X.D /= 99 or not Plain.Matches (X)
            or Plain.Definite or Plain_Nested (X) /= 3 then
            raise Program_Error;
        end if;
        if Defaults.Read (Y) /= 3 or Y.D /= 88 or not Defaults.Matches (Y)
            or not Defaults.Definite or Default_Nested (Y) /= 13 then
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
    Ada.Text_IO.Put_Line ("generic discriminants passed");
end GenericDiscriminants;
