with Ada.Text_IO;
procedure GenericContractRegions is
    generic
        type T is private;
        type U is private;
    package Model is
        function Run (Left : T; Right : U) return Integer;
    private
        function Select_Value (X : T) return Integer;
        function Select_Value (X : U) return Integer;
    end Model;
    package body Model is
        function Select_Value (X : T) return Integer is
        begin
            return 1;
        end Select_Value;
        function Select_Value (X : U) return Integer is
        begin
            return 2;
        end Select_Value;
        function Local (X : T) return Integer is
        begin
            return 3;
        end Local;
        function Local (X : U) return Integer is
        begin
            return 4;
        end Local;
        package Body_Part is
            type Tag is tagged null record;
            function Choose (Object : Tag; X : T) return Integer;
            function Choose (Object : Tag; X : U) return Integer;
        end Body_Part;
        package body Body_Part is
            function Choose (Object : Tag; X : T) return Integer is
            begin
                return 5;
            end Choose;
            function Choose (Object : Tag; X : U) return Integer is
            begin
                return 6;
            end Choose;
        end Body_Part;
        function Run (Left : T; Right : U) return Integer is
            Object : Body_Part.Tag;
        begin
            return Select_Value (Left) * 100000 + Select_Value (Right) * 10000
                + Local (Left) * 1000 + Local (Right) * 100
                + Body_Part.Choose (Object, Left) * 10 + Body_Part.Choose (Object, Right);
        end Run;
    end Model;
    package Collapsed is new Model (Integer, Integer);
    package Distinct is new Model (Integer, Boolean);
    type Pointer is access Integer;
    subtype NN is not null Pointer;
    generic
        Object : in out NN;
    package Exclusion is
        function Read return Integer;
    private
        Alias_Value : not null Pointer renames Object;
    end Exclusion;
    package body Exclusion is
        function Read return Integer is
        begin
            return Alias_Value.all;
        end Read;
    end Exclusion;
    procedure Check is
        Value : NN := new Integer'(5);
        package View is new Exclusion (Value);
    begin
        if View.Read /= 5 then raise Program_Error; end if;
        Value := new Integer'(6);
        if View.Read /= 6 then raise Program_Error; end if;
        if Collapsed.Run (1, 2) /= 123456 or Distinct.Run (1, True) /= 123456 then
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
    Ada.Text_IO.Put_Line ("generic contract regions passed");
end GenericContractRegions;
