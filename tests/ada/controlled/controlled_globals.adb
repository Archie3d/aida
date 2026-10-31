with Ada.Text_IO; use Ada.Text_IO;
package body Controlled_Globals is
    Last : Guard;
    package Nested is
        Item : Guard;
    end Nested;
begin
    Put_Line ("globals ready" & Integer'Image (Live));
end Controlled_Globals;
