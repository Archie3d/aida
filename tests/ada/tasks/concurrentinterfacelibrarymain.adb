with Concurrent_Interface_Model; use Concurrent_Interface_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ConcurrentInterfaceLibraryMain is
    procedure Add_Task is new Apply (Worker);
    procedure Add_Protected is new Apply (Counter);
    procedure Run is
        T : Worker (7);
        P : Counter (3);
        Value : Integer := 0;
    begin
        for I in 1 .. 3 loop
            Add_Task (T, Value);
            Add_Protected (P, Value);
        end loop;
        if Value /= 30 then raise Program_Error; end if;
    end Run;
    task type Client;
    task body Client is
    begin
        Run;
    exception
        when others => Put_Line ("interface client failed");
    end Client;
begin
    Run;
    declare
        Clients : array (1 .. 4) of Client;
    begin
        null;
    end;
    Put_Line ("interface library OK");
end ConcurrentInterfaceLibraryMain;
