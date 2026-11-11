with Ada.Text_IO;
with Ada.Finalization;
procedure Controlleddependentarrays is
    procedure Run is
        Live, Adjusts, Next_Token : Integer := 0;
        Acquire_Budget : Integer := -1;
        Fail_Adjust : Boolean := False;
        function Acquire return Integer is
        begin
            if Acquire_Budget = 0 then
                raise Constraint_Error;
            end if;
            if Acquire_Budget > 0 then
                Acquire_Budget := Acquire_Budget - 1;
            end if;
            Next_Token := Next_Token + 1;
            Live := Live + 1;
            return Next_Token;
        end Acquire;
        package Model is
            type Cell is new Ada.Finalization.Controlled with record
                Token : Integer := Acquire;
                Value : Integer := 7;
            end record;
            overriding procedure Adjust (Object : in out Cell);
            overriding procedure Finalize (Object : in out Cell);
            type Items is array (Positive range <>) of Cell;
            type Bundle (Count : Natural) is record
                Data : Items (1 .. Count);
            end record;
        end Model;
        package body Model is
            overriding procedure Adjust (Object : in out Cell) is
            begin
                Adjusts := Adjusts + 1;
                if Fail_Adjust then
                    Fail_Adjust := False;
                    raise Constraint_Error;
                end if;
                Object.Token := Acquire;
            end Adjust;
            overriding procedure Finalize (Object : in out Cell) is
            begin
                if Object.Token = 0 then
                    raise Program_Error with "duplicate finalization";
                end if;
                Object.Token := 0;
                Live := Live - 1;
            end Finalize;
        end Model;
        procedure Check (Value : Boolean) is
        begin
            if not Value then
                raise Program_Error with "controlled dependent arrays";
            end if;
        end Check;
    begin
        declare
            Empty : Model.Bundle (0);
            Defaulted : Model.Bundle (3);
            Aggregate_Value : Model.Bundle (2) :=
                (2, (others => (Ada.Finalization.Controlled with Token => Acquire, Value => 11)));
        begin
            Check (Live = 5 and Adjusts = 0);
            declare
                Copy : Model.Bundle := Aggregate_Value;
            begin
                Check (Live = 7 and Adjusts = 2);
                Copy.Data (1).Value := 99;
                Check (Aggregate_Value.Data (1).Value = 11);
                Copy := Aggregate_Value;
                Check (Live = 7 and Adjusts = 4 and Copy.Data (1).Value = 11);
            end;
            Check (Live = 5);
            Fail_Adjust := True;
            begin
                declare
                    Failed_Copy : Model.Bundle := Defaulted;
                begin
                    raise Program_Error with "Adjust did not fail";
                end;
            exception
                when Program_Error => null;
            end;
            Check (Live = 5 and Adjusts = 7 and not Fail_Adjust);
        end;
        Check (Live = 0);
        Acquire_Budget := 2;
        begin
            declare
                Partial : Model.Bundle (4);
            begin
                raise Program_Error with "initialization did not fail";
            end;
        exception
            when Constraint_Error => null;
        end;
        Check (Live = 0);
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("controlled dependent array worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("controlled dependent arrays passed");
end Controlleddependentarrays;
