with Ada.Text_IO;
procedure GenericPackageContracts is
    procedure Check is
        function Next (Value : Integer) return Integer is
        begin
            return Value + 1;
        end Next;
        function Alias_Next (Value : Integer) return Integer renames Next;
        Counter : Integer := 0;
        generic
            type T is private;
            Start : Integer := 3;
            with function Step (Value : Integer) return Integer is Next;
            Target : in out Integer;
        package Model is
            subtype Element is T;
            function Read return Integer;
            procedure Advance;
        end Model;
        package body Model is
            function Read return Integer is
            begin
                return Step (Start) + Target;
            end Read;
            procedure Advance is
            begin
                Target := Step (Target);
            end Advance;
        end Model;
        generic package Renamed_Model renames Model;
        package Data is new Model (Integer, Target => Counter);
        package Alias_Data renames Data;
        generic
            type Item is private;
            Initial : Integer;
            Variable : in out Integer;
            with package P is new Renamed_Model (Item, Initial, Alias_Next, Variable);
        package Consumer is
            function Read return Integer;
            procedure Advance;
            generic
                with package Q is new Model (Item, Initial, Next, Variable);
            package Inner is
                function Read return Integer;
            end Inner;
        end Consumer;
        package body Consumer is
            function Read return Integer is
            begin
                return P.Read;
            end Read;
            procedure Advance is
            begin
                P.Advance;
            end Advance;
            package body Inner is
                function Read return Integer is
                begin
                    return Q.Read;
                end Read;
            end Inner;
        end Consumer;
        package View is new Consumer (Integer, 1 + 2, Counter, Alias_Data);
        package Nested is new View.Inner (Data);
        generic
            with package P is new Model (Integer, Target => Counter);
        package Defaults is
            function Read return Integer;
        end Defaults;
        package body Defaults is
            function Read return Integer is
            begin
                return P.Read;
            end Read;
        end Defaults;
        package Default_View is new Defaults (Data);
    begin
        if View.Read /= 4 or Nested.Read /= 4 or Default_View.Read /= 4 then
            raise Program_Error;
        end if;
        View.Advance;
        if Counter /= 1 or Data.Read /= 5 then
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
    Ada.Text_IO.Put_Line ("formal package contracts passed");
end GenericPackageContracts;
