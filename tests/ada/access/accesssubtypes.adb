with Ada.Text_IO;
with Access_Subtype_Checks;
procedure AccessSubtypes is
    Secret : Access_Subtype_Checks.Secret_Fixed := Access_Subtype_Checks.Make_Secret (False);
    protected Results is
        procedure Passed;
        function Count return Integer;
    private
        Good : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        function Count return Integer is
        begin
            return Good;
        end Count;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
        begin
            for I in 1 .. 10 loop
                Access_Subtype_Checks.Run;
            end loop;
            Results.Passed;
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
begin
    if Secret.D /= 2 then raise Program_Error; end if;
    begin
        Secret := Access_Subtype_Checks.Secret_Fixed (Access_Subtype_Checks.Make_Secret (True));
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Access_Subtype_Checks.Run;
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("access subtype constraints passed");
end AccessSubtypes;
