with Ada.Command_Line; use Ada.Command_Line;
with Ada.Text_IO;
with CommandLineStartup;
procedure CommandLine is
    -- These calls happen during elaboration, before the main body's statements.
    count : Natural := Argument_Count;
    command : String := Command_Name;
    procedure check (condition : Boolean) is
    begin
        if not condition then
            raise Program_Error;
        end if;
    end check;
begin
    check (CommandLineStartup.Count = count and CommandLineStartup.Name_Length = command'Length);
    check (command'First = 1 and command'Length > 0);
    if count = 0 then
        Set_Exit_Status (Failure);
        Set_Exit_Status (Success);
    else
        check (count = 4);
        check (Argument (1) = "hello world");
        check (Argument (2) = "");
        check (Argument (3) = "--literal");
        check (Argument (4) = command);
        check (Argument (1)'First = 1);
        Set_Exit_Status (42);
    end if;
    begin
        Ada.Text_IO.Put_Line (Argument (count + 1));
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Ada.Text_IO.Put_Line ("commandline: passed");
end CommandLine;
