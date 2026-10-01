with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledParts is
    Live : Integer := 0;
    Initializes : Integer := 0;
    Adjusts : Integer := 0;
    Fail_Init : Integer := 0;
    Fail_Adjust : Integer := 0;
    Fail_Finalize : Boolean := False;
    type Guard is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    procedure Initialize (Object : in out Guard) is
    begin
        Initializes := Initializes + 1;
        if Initializes = Fail_Init then
            raise Constraint_Error;
        end if;
        Live := Live + 1;
    end Initialize;
    procedure Adjust (Object : in out Guard) is
    begin
        Adjusts := Adjusts + 1;
        if Adjusts = Fail_Adjust then
            raise Constraint_Error;
        end if;
        Live := Live + 1;
    end Adjust;
    procedure Finalize (Object : in out Guard) is
    begin
        Live := Live - 1;
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
    type Guards is array (1 .. 3) of Guard;
    type Parent is new Ada.Finalization.Controlled with record
        Child : Guards;
    end record;
    overriding procedure Initialize (Object : in out Parent) is
    begin
        Check (Live = 3);
        raise Constraint_Error;
    end Initialize;
    overriding procedure Finalize (Object : in out Parent) is
    begin
        Put_Line ("wrong parent finalization");
    end Finalize;
begin
    Fail_Init := 2;
    begin
        declare
            A : Guards;
        begin
            Put_Line ("wrong array initialization");
        end;
    exception
        when Constraint_Error => Check (Live = 0);
    end;
    Fail_Init := 0;
    Put_Line ("partial initialization");
    begin
        declare
            A : Parent;
        begin
            Put_Line ("wrong parent initialization");
        end;
    exception
        when Constraint_Error => Check (Live = 0);
    end;
    Put_Line ("parent initialization failure");
    declare
        Source : Guards;
    begin
        Fail_Adjust := Adjusts + 2;
        begin
            declare
                Copy : Guards := Source;
            begin
                Put_Line ("wrong copied initialization");
            end;
        exception
            when Program_Error => Check (Live = 3 and Adjusts = Fail_Adjust + 1);
        end;
    end;
    Check (Live = 0);
    Put_Line ("partial adjustment");
    declare
        Source, Target : Guards;
    begin
        Fail_Adjust := Adjusts + 5;
        begin
            Target := Source;
        exception
            when Program_Error => Check (Live = 5 and Adjusts = Fail_Adjust + 1);
        end;
        Fail_Adjust := 0;
        Target := Source;
        Check (Live = 6);
    end;
    Check (Live = 0);
    Put_Line ("assignment adjustment failure and recovery");
    declare
        Source, Target : Guards;
    begin
        Fail_Finalize := True;
        begin
            Target := Source;
        exception
            when Program_Error => Check (Live = 3);
        end;
        Fail_Finalize := False;
    end;
    Check (Live = 0);
    Put_Line ("assignment finalization failure");
    Put_Line ("controlled parts passed");
end ControlledParts;
