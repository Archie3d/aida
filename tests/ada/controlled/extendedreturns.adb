with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure ExtendedReturns is
    Calls : Integer := 0;
    Finalized : Integer := 0;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    function Count return Integer is
    begin
        Calls := Calls + 1;
        return 3;
    end Count;
    function Scalar return Integer is
    begin
        return Result : aliased Integer := Count do
            declare
                type Guard is new Ada.Finalization.Limited_Controlled with null record;
                overriding procedure Finalize (Object : in out Guard);
                overriding procedure Finalize (Object : in out Guard) is
                begin
                    Finalized := Finalized + 1;
                end Finalize;
                Local : Guard;
            begin
                Result := Result + 2;
                return;
            end;
        end return;
    end Scalar;
    function Text (N : Integer) return String is
    begin
        return Result : String (2 .. N + 1) := (others => 'x') do
            Result (2) := 'y';
        end return;
    end Text;
    subtype Three is String (1 .. 3);
    function Fixed_Text return Three is
    begin
        return Result : Three := "abc";
    end Fixed_Text;
    function Inferred return String is
    begin
        return Result : String := Text (3) do
            Check (Result'First = 2 and Result'Last = 4);
        end return;
    end Inferred;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    function Grid (N : Integer) return Matrix is
    begin
        return Result : Matrix (2 .. N + 1, 5 .. 6) := (others => (others => 7)) do
            Result (2, 5) := 8;
        end return;
    end Grid;
    type Plain is record
        Value : Integer := 9;
    end record;
    function Record_Result return Plain is
    begin
        return Result : Plain do
            Result.Value := Result.Value + 1;
        end return;
    end Record_Result;
    type Tagged_Item is tagged record
        Value : Integer := 1;
    end record;
    function Specific return Tagged_Item is
    begin
        return Result : Tagged_Item do
            Result.Value := 2;
        end return;
    end Specific;
    function Wide return Tagged_Item'Class is
    begin
        return Result : Tagged_Item'Class := Specific do
            Result.Value := 3;
        end return;
    end Wide;
    function Recovered return Integer is
    begin
        return Result : Integer := 1 do
            raise Constraint_Error;
        exception
            when Constraint_Error =>
                Result := 12;
                return;
        end return;
    end Recovered;
    function Fail return Integer is
    begin
        raise Constraint_Error;
        return 0;
    end Fail;
    function Failed_Initializer return Integer is
    begin
        return Result : Integer := Fail do
            Check (False);
        exception
            when Constraint_Error => Check (False);
        end return;
    exception
        when Constraint_Error => return 13;
    end Failed_Initializer;
    function Abandoned (Use_Exit : Boolean) return Integer is
    begin
        for I in 1 .. 1 loop
            return Result : Integer := 99 do
                if Use_Exit then
                    exit;
                else
                    goto Later;
                end if;
            end return;
        end loop;
    <<Later>>
        return Result : Integer := 14;
    end Abandoned;
    type Modulus is mod 256;
    function Modular_Result return Modulus is
    begin
        return Result : Modulus := 255 do
            Result := Result + 2;
        end return;
    end Modular_Result;
    type Fixed is delta 0.125 range -10.0 .. 10.0;
    function Fixed_Result return Fixed is
    begin
        return Result : Fixed := 1.25 do
            Result := Result + 0.5;
        end return;
    end Fixed_Result;
    function Real_Result return Float is
    begin
        return Result : Float := 1.5 do
            Result := Result * 2.0;
        end return;
    end Real_Result;
    generic
        type Number is range <>;
    function Generic_Result (Value : Number) return Number;
    function Generic_Result (Value : Number) return Number is
    begin
        return Result : Number := Value do
            Result := Result + 1;
        end return;
    end Generic_Result;
    function Next is new Generic_Result (Integer);
    task type Worker;
    task body Worker is
        Value : Matrix := Grid (3);
    begin
        Check (Value (2, 5) = 8 and Text (2) = "yx" and Next (8) = 9);
    end Worker;
    Value : Matrix := Grid (3);
    Object : Tagged_Item'Class := Wide;
begin
    Check (Scalar = 5 and Calls = 1 and Finalized = 1);
    Check (Text (3) = "yxx" and Fixed_Text = "abc" and Inferred = "yxx");
    Check (Value'First (1) = 2 and Value'Last (1) = 4 and Value'First (2) = 5);
    Check (Value (2, 5) = 8 and Value (3, 6) = 7);
    Check (Record_Result.Value = 10 and Object.Value = 3);
    Check (Recovered = 12 and Failed_Initializer = 13);
    Check (Abandoned (True) = 14 and Abandoned (False) = 14);
    Check (Modular_Result = 1 and Fixed_Result = 1.75 and Real_Result = 3.0);
    Check (Next (5) = 6);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Put_Line ("extended returns passed");
end ExtendedReturns;
