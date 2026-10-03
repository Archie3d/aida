with Controlled_Completion_Model; use Controlled_Completion_Model;
with Controlled_Result_Model; use Controlled_Result_Model;
with System;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledCompletion is
    use type System.Address;
begin
    Check (Live = 1 and Objects = 2 and Resources = 2);
    Check (Limited_Object.Original = Limited_Object'Address);
    Check (Child (Dynamic_Object).Item.Data.Value = 42 and Child (Heap.all).Item.Data.Value = 42);
    declare
        Local : Root'Class := Dynamic;
        Fixed : Limited_Guard := Controlled_Completion_Model.Make;
    begin
        Check (Live = 2 and Objects = 3);
        Check (Fixed.Original = Fixed'Address);
    end;
    Check (Live = 1 and Objects = 2);
    Put_Line ("completion globals alive");
end ControlledCompletion;
