with Ada.Text_IO; use Ada.Text_IO;
procedure NullExclusionTasking is
    Item : aliased Integer := 42;
    type Link is access all Integer;
    subtype Required_Link is not null Link;
    protected Results is
        procedure Finished;
        function Count return Integer;
    private
        Total : Integer := 0;
    end Results;
    protected body Results is
        procedure Finished is
        begin
            Total := Total + 1;
        end Finished;
        function Count return Integer is
        begin
            return Total;
        end Count;
    end Results;
    task type Worker;
    task body Worker is
        P : Required_Link := Item'Access;
        Failures : Integer := 0;
        procedure Take (Value : not null access Integer) is
        begin
            if Value.all /= 42 then raise Program_Error; end if;
        end Take;
        procedure Change (Value : in out Required_Link) is
        begin
            Value := Item'Access;
        end Change;
    begin
        for Iteration in 1 .. 30 loop
            Take (P);
            Change (P);
            begin
                Take (null);
                raise Program_Error;
            exception
                when Constraint_Error => Failures := Failures + 1;
            end;
            begin
                P := null;
                raise Program_Error;
            exception
                when Constraint_Error => Failures := Failures + 1;
            end;
        end loop;
        if Failures = 60 and P.all = 42 then Results.Finished; end if;
    end Worker;
    type Workers is array (1 .. 4) of Worker;
begin
    declare
        Tasks : Workers;
    begin
        null;
    end;
    if Results.Count /= 4 then raise Program_Error; end if;
    Put_Line ("task null exclusions passed");
end NullExclusionTasking;
