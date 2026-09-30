with Ada.Text_IO; use Ada.Text_IO;
package body Controlled_Model is
    procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
        Object.Id := Live;
        Put_Line ("library initialize" & Integer'Image (Object.Id));
    end Initialize;
    procedure Finalize (Object : in out Guard) is
    begin
        Put_Line ("library finalize" & Integer'Image (Object.Id));
        Live := Live - 1;
    end Finalize;
end Controlled_Model;
