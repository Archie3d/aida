-- Host argv bytes are exposed unchanged. Status codes are portable 0 .. 255.
package Ada.Command_Line is
    function Argument_Count return Natural;
    pragma Import (C, Argument_Count, "__ada_argument_count");
    function Argument (Number : Positive) return String;
    pragma Import (C, Argument, "__ada_argument");
    function Command_Name return String;
    pragma Import (C, Command_Name, "__ada_command_name");

    type Exit_Status is range 0 .. 255;
    Success : constant Exit_Status := 0;
    Failure : constant Exit_Status := 1;
    procedure Set_Exit_Status (Code : Exit_Status);
    pragma Import (C, Set_Exit_Status, "__ada_set_exit_status");
end Ada.Command_Line;
