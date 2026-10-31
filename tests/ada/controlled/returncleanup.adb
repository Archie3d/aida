with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure ReturnCleanup is
    Live : Integer := 0;
    Finalized : Integer := 0;
    Fail_Init : Boolean := False;
    Fail_Finalize : Boolean := False;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
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
        if Fail_Finalize then
            Fail_Finalize := False;
            raise Constraint_Error;
        end if;
    end Finalize;
    function Initial_Value return Integer is
    begin
        if Fail_Init then
            Fail_Init := False;
            raise Constraint_Error;
        end if;
        return 0;
    end Initial_Value;
    type Envelope is limited record
        Resource : Guard;
        Value : Integer := Initial_Value;
    end record;
    function Construct (Mode : Integer) return Envelope is
    begin
        for I in 1 .. 1 loop
            return Result : Envelope do
                if Mode = 1 then
                    exit;
                elsif Mode = 2 then
                    goto Retry;
                elsif Mode = 3 then
                    raise Constraint_Error;
                elsif Mode = 4 then
                    declare
                        task Worker;
                        task body Worker is
                        begin
                            Result.Value := 5;
                        end Worker;
                    begin
                        return;
                    end;
                elsif Mode = 5 then
                    Fail_Finalize := True;
                    exit;
                end if;
            end return;
        end loop;
    <<Retry>>
        return Result : Envelope;
    exception
        when Constraint_Error | Program_Error => return Result : Envelope;
    end Construct;
    function Failed_Text return String is
        Local : Guard;
    begin
        return Result : String (1 .. 3) := "abc" do
            Fail_Finalize := True;
        end return;
    end Failed_Text;
    function Caught return Envelope is
    begin
        return Result : Envelope do
            raise Constraint_Error;
        exception
            when Constraint_Error =>
                Result.Value := 7;
                return;
        end return;
    end Caught;
begin
    for Mode in 0 .. 6 loop
        Finalized := 0;
        if Mode = 6 then
            Fail_Init := True;
        end if;
        declare
            Object : Envelope := Construct (Mode);
        begin
            Check (Live = 1);
            if Mode = 4 then
                Check (Object.Value = 5);
            end if;
        end;
        Check (Live = 0);
        if Mode = 0 or Mode = 4 then
            Check (Finalized = 1);
        else
            Check (Finalized = 2);
        end if;
    end loop;
    declare
        Object : Envelope := Caught;
    begin
        Check (Live = 1 and Object.Value = 7);
    end;
    Check (Live = 0);
    begin
        Put_Line (Failed_Text);
        Check (False);
    exception
        when Constraint_Error | Program_Error => Check (Live = 0);
    end;
    Put_Line ("return cleanup passed");
end ReturnCleanup;
