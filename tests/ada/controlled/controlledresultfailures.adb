with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledResultFailures is
    function Retry return Pair is
        Local : Pair;
    begin
        Fail_Adjust := 2;
        return Local;
    exception
        when Program_Error =>
            Check (Objects = 2 and Resources = 2);
            return Local;
    end Retry;
    function Failed_Cleanup return Guard is
        Local : Guard;
    begin
        Fail_Finalize := True;
        return Local;
    end Failed_Cleanup;
    function Failed_Adjust return Guards is
        Local : Guards (1 .. 3);
    begin
        Fail_Adjust := 2;
        return Local;
    end Failed_Adjust;
    function Missing return Guard is
        Local : Guard;
    begin
        null;
    end Missing;
begin
    declare
        Result : Pair := Retry;
    begin
        Check (Objects = 2 and Resources = 2);
    end;
    Check (Objects = 0 and Resources = 0);
    begin
        Fail_Adjust := 2;
        declare
            Result : Guard := Make (42);
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Check (Objects = 0 and Resources = 0);
    end;
    begin
        declare
            Result : Guard := Failed_Cleanup;
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Check (Objects = 0 and Resources = 0);
    end;
    begin
        declare
            Result : Guards := Failed_Adjust;
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Check (Objects = 0 and Resources = 0);
    end;
    begin
        declare
            Result : Guard := Missing;
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Check (Objects = 0 and Resources = 0);
    end;
    Put_Line ("controlled result failures passed");
end ControlledResultFailures;
