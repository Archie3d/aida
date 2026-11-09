with Ada.Text_IO; use Ada.Text_IO;
with Ada.Task_Identification; use Ada.Task_Identification;
procedure TaskEntryQueues is
    protected Results is
        procedure Success;
        procedure Failure;
        function Check return Boolean;
    private
        Good : Integer := 0;
        Bad : Integer := 0;
    end Results;
    protected body Results is
        procedure Success is begin Good := Good + 1; end Success;
        procedure Failure is begin Bad := Bad + 1; end Failure;
        function Check return Boolean is begin return Good = 2 and Bad = 2; end Check;
    end Results;
    task S is
        entry Work (Caller : Task_Id; Value : out Integer);
    end S;
    task body S is
    begin
        while Work'Count < 4 loop delay 0.0; end loop;
        for I in 1 .. 2 loop
            accept Work (Caller : Task_Id; Value : out Integer) do
                if Work'Caller /= Caller then raise Program_Error; end if;
                Value := I;
            end Work;
        end loop;
    end S;
    task type Client;
    task body Client is
        Value : Integer := 0;
    begin
        S.Work (Current_Task, Value);
        if Value not in 1 .. 2 then raise Program_Error; end if;
        Results.Success;
    exception
        when Tasking_Error => Results.Failure;
    end Client;
begin
    declare Clients : array (1 .. 4) of Client;
    begin null; end;
    if not Results.Check then raise Program_Error; end if;
    while not S'Terminated loop delay 0.0; end loop;
    declare Value : Integer;
    begin
        S.Work (Current_Task, Value);
        raise Program_Error;
    exception when Tasking_Error => null; end;
    Put_Line ("queues OK");
end TaskEntryQueues;
