with Ada.Text_IO; use Ada.Text_IO;
package body Controlled_Completion_Model is
    procedure Initialize (Object : in out Limited_Guard) is
    begin
        Live := Live + 1;
        Object.Original := Object'Address;
    end Initialize;
    procedure Finalize (Object : in out Limited_Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    function Make return Limited_Guard is
    begin
        return Object : Limited_Guard;
    end Make;
    function Dynamic return Root'Class is
        Object : Child;
    begin
        Object.Item.Data.Value := 42;
        return Object;
    end Dynamic;
    procedure Finalize (Object : in out Audit) is
    begin
        Controlled_Result_Model.Check (Live = 0 and Controlled_Result_Model.Objects = 0 and Controlled_Result_Model.Resources = 0);
        Put_Line ("completion globals finalized");
    end Finalize;
end Controlled_Completion_Model;
