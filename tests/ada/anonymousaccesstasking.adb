with Ada.Text_IO; use Ada.Text_IO;
procedure AnonymousAccessTasking is
    Global : aliased Integer := 7;
    type Link is access all Integer;
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
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Constraint_Error; end if;
    end Check;
    procedure Capture (P : access Integer) is
        Value : Link := Link (P);
    begin
        Check (Value.all = 7);
    end Capture;
    task type Worker;
    task body Worker is
        Local : aliased Integer := 3;
        type Local_Link is access all Integer;
        procedure Use_Local (P : access Integer) is
            Value : Local_Link := Local_Link (P);
        begin
            Check (Value.all = 3);
        end Use_Local;
        procedure Forward (P : access Integer) is
        begin
            Capture (P);
        end Forward;
    begin
        for Iteration in 1 .. 30 loop
            Forward (Global'Access);
            Use_Local (Local'Access);
            begin
                Forward (Local'Access);
                raise Constraint_Error;
            exception
                when Program_Error => null;
            end;
        end loop;
        Results.Finished;
    end Worker;
    type Workers is array (1 .. 4) of Worker;
begin
    declare
        Tasks : Workers;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Put_Line ("anonymous task accessibility passed");
end AnonymousAccessTasking;
