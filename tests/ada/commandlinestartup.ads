with Ada.Command_Line;
package CommandLineStartup is
    Count : constant Natural := Ada.Command_Line.Argument_Count;
    Name_Length : constant Natural := Ada.Command_Line.Command_Name'Length;
end CommandLineStartup;
