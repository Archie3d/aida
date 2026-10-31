with Ada.Finalization;
package body Elaboration_Blocks is
    Live : Integer := 0;
    Finalized : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Live := Live - 1;
        Finalized := Finalized + 1;
    end Finalize;
    function Fail return Integer is
    begin
        raise Constraint_Error;
        return 0;
    end Fail;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    -- Ordinary stack locals also work when no nested routine captures them.
    declare
        type Pair is record
            Left : Integer;
            Right : Integer;
        end record;
        Value : Pair := (Left => 2, Right => 3);
        subtype Small is Integer range 1 .. Value.Right;
        Number : Small := Value.Left;
    begin
        Check (Number + Value.Right = 5);
    end;
    for Iteration in 1 .. 3 loop
        declare
            Value : Integer := Iteration;
            Text : String (1 .. Value) := (others => 'a');
            Object : Guard;
            type Link is access Guard;
            Allocated : Link := new Guard;
            procedure Increment is
                procedure Inner is
                begin
                    Value := Value + Text'Length;
                end Inner;
            begin
                Inner;
            end Increment;
            type Callback is access procedure;
            Action : Callback := Increment'Access;
            package Local is
                Seed : Integer := Value;
                procedure Run;
            end Local;
            package body Local is
                procedure Run is
                begin
                    Action.all;
                end Run;
            begin
                declare
                    Temporary : Guard;
                begin
                    Check (Live = 3);
                end;
                Check (Live = 2);
            end Local;
        begin
            Local.Run;
            Check (Value = Iteration * 2 and Local.Seed = Iteration);
            Check (Allocated /= null and Text (1) = 'a');
        end;
        Check (Live = 0);
    end loop;
    Check (Finalized = 9);
    -- A block's handler retains its declarations, then finalizes them.
    declare
        Object : Guard;
    begin
        raise Constraint_Error;
    exception
        when Constraint_Error => Check (Live = 1);
    end;
    Check (Live = 0 and Finalized = 10);
    -- Declaration failure bypasses that block's handlers.
    begin
        declare
            Object : Guard;
            Bad : Integer := Fail;
        begin
            raise Program_Error;
        exception
            when Constraint_Error => raise Program_Error;
        end;
    exception
        when Constraint_Error => Check (Live = 0 and Finalized = 11);
    end;
    -- A loop transfer also cleans the block and its access collection.
    loop
        declare
            Object : Guard;
            type Link is access Guard;
            Allocated : Link := new Guard;
        begin
            exit;
        end;
    end loop;
    Check (Live = 0 and Finalized = 13);
    declare
        Value : Integer := 4;
        type Local_Guard is new Ada.Finalization.Controlled with null record;
        overriding procedure Finalize (Object : in out Local_Guard) is
        begin
            Check (Value = 4);
            Finalized := Finalized + Value;
        end Finalize;
        Object : Local_Guard;
    begin
        raise Constraint_Error;
    end;
exception
    when Constraint_Error =>
        Check (Live = 0 and Finalized = 17);
        declare
            Seen : Integer := Finalized;
            function Captured return Integer is
            begin
                return Seen;
            end Captured;
        begin
            Finished := Captured = 17;
        end;
end Elaboration_Blocks;
