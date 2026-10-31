with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure GotoCleanup is
    Trace : Integer := 0;
    Completed : Boolean := False;
    Fail_Finalize : Boolean := False;
    type Guard is new Ada.Finalization.Limited_Controlled with record
        Id : Integer := 0;
    end record;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Trace := Trace * 10 + Object.Id;
        if Fail_Finalize then
            raise Constraint_Error;
        end if;
    end Finalize;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    function Length return Integer is
    begin
        return 10000;
    end Length;
    function Build return Guard is
        Attempts : Integer := 0;
        Steps : Integer := 0;
    begin
        <<Retry_Result>>
        Attempts := Attempts + 1;
        return Result : Guard do
            Result.Id := 6;
            <<Within_Result>>
            Steps := Steps + 1;
            if Steps < 2 then
                goto Within_Result;
            end if;
            if Attempts = 1 then
                goto Retry_Result;
            end if;
        end return;
    end Build;
    Iteration : Integer := 0;
begin
    declare
        Outer_Guard : Guard := (Ada.Finalization.Limited_Controlled with Id => 1);
    begin
        <<Retry>>
        Iteration := Iteration + 1;
        declare
            Inner_Guard : Guard := (Ada.Finalization.Limited_Controlled with Id => 2);
            Text : String (1 .. Length) := (others => 'x');
        begin
            Check (Text (Text'Last) = 'x');
            if Iteration < 3 then
                goto Retry;
            end if;
            declare
                Last_Guard : Guard := (Ada.Finalization.Limited_Controlled with Id => 3);
                task Worker;
                task body Worker is
                begin
                    Completed := True;
                end Worker;
            begin
                goto Done;
            end;
        exception
            when others => raise Program_Error;
        end;
    end;
    raise Program_Error;
    <<Done>>
    Check (Trace = 22321);
    Check (Completed);
    -- A failure after leaving a handled block belongs to the destination.
    begin
        begin
            goto Outside_Handler;
        exception
            when others => raise Program_Error;
        end;
        <<Outside_Handler>>
        raise Constraint_Error;
    exception
        when Constraint_Error => null;
    end;
    Trace := 0;
    declare
        type Link is access Guard;
        Pointer : Link := new Guard'(Ada.Finalization.Limited_Controlled with Id => 4);
    begin
        goto After_Collection;
    end;
    <<After_Collection>>
    Check (Trace = 4);
    Fail_Finalize := True;
    begin
        declare
            Failing : Guard := (Ada.Finalization.Limited_Controlled with Id => 5);
        begin
            goto Finalizer_Destination;
        exception
            when others => raise Storage_Error;
        end;
        <<Finalizer_Destination>>
        raise Storage_Error;
    exception
        when Program_Error => null;
    end;
    Check (Trace = 45);
    Fail_Finalize := False;
    Trace := 0;
    declare
        Result : Guard := Build;
    begin
        Check (Trace = 6);
        Check (Result.Id = 6);
    end;
    Check (Trace = 66);
    Put_Line ("goto cleanup passed");
end GotoCleanup;
