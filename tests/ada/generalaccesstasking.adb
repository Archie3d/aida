with Ada.Text_IO; use Ada.Text_IO;
procedure GeneralAccessTasking is
    Text : aliased String := "shared";
    type String_Link is access all String;
    Shared : String_Link := Text'Access;
    protected Results is
        procedure Finished;
        function Count return Integer;
    private
        Completed : Integer := 0;
    end Results;
    protected body Results is
        procedure Finished is
        begin
            Completed := Completed + 1;
        end Finished;
        function Count return Integer is
        begin
            return Completed;
        end Count;
    end Results;
    task type Worker;
    task body Worker is
        type Local_Link is access constant String;
        Local : Local_Link := Text'Access;
        View : String_Link;
    begin
        for Iteration in 1 .. 100 loop
            View := Text'Access;
            if View.all /= "shared" or Local.all /= "shared"
                or View'Length /= 6 or View /= Shared then
                raise Program_Error;
            end if;
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
    if Results.Count /= 4 or Shared.all /= "shared" then
        raise Program_Error;
    end if;
    Put_Line ("general access tasking passed");
end GeneralAccessTasking;
