with Ada.Text_IO;
with Return_Model; use Return_Model;
procedure GenericLimitedObjects is
    procedure Check is
        Calls : Integer := 0;
        function Build (N : Integer) return Cell is
        begin
            Calls := Calls + 1;
            return Forward (N);
        end Build;
        generic
            Value : Cell := Build (13);
        package Model is
            function Read return Integer;
            function Valid return Boolean;
        end Model;
        package body Model is
            function Read return Integer is
            begin
                return Value.Value;
            end Read;
            function Valid return Boolean is
            begin
                return Value.Self.all'Address = Value.Value'Address;
            end Valid;
        end Model;
        package Defaulted is new Model;
        package Explicit is new Model (Cell'(Build (21)));
        generic
            Value : Pair;
        function Sum return Integer;
        function Sum return Integer is
        begin
            if Value.First.Self.all'Address /= Value.First.Value'Address
                or Value.Second.Self.all'Address /= Value.Second.Value'Address then
                raise Program_Error;
            end if;
            return Value.First.Value + Value.Second.Value;
        end Sum;
        function Read_Pair is new Sum ((First => Build (3), Second => Build (5)));
        generic
            type T is limited private;
            with function Create return T;
            with function Valid (Value : T) return Boolean;
        package Outer is
            generic
                Value : T := Create;
            function Test return Boolean;
        end Outer;
        package body Outer is
            function Test return Boolean is
            begin
                return Valid (Value);
            end Test;
        end Outer;
        function Make_Cell return Cell is
        begin
            return Build (7);
        end Make_Cell;
        function Valid_Cell (Value : Cell) return Boolean is
        begin
            return Value.Value = 7 and Value.Self.all'Address = Value.Value'Address;
        end Valid_Cell;
        package Nested is new Outer (Cell, Make_Cell, Valid_Cell);
        function Test_Nested is new Nested.Test;
    begin
        if Calls /= 5 or Defaulted.Read /= 13 or Explicit.Read /= 21
            or not Defaulted.Valid or not Explicit.Valid or Read_Pair /= 8
            or not Test_Nested then
            raise Program_Error;
        end if;
        if Calls /= 5 then raise Program_Error; end if;
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
    Ada.Text_IO.Put_Line ("GenericLimitedObjects passed");
end GenericLimitedObjects;
