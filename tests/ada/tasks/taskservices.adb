with Ada.Text_IO; use Ada.Text_IO;
with Ada.Task_Attributes;
with Ada.Task_Identification; use Ada.Task_Identification;
with Ada.Synchronous_Task_Control; use Ada.Synchronous_Task_Control;
procedure TaskServices is
    package Ints is new Ada.Task_Attributes (Integer, 7);
    use type Ints.Attribute_Handle;
    Gate, Done : Suspension_Object;
    task Worker;
    task body Worker is
    begin
        if Ints.Value /= 7 then raise Program_Error; end if;
        Ints.Set_Value (42);
        Suspend_Until_True (Gate);
        if Ints.Value /= 42 then raise Program_Error; end if;
        Set_True (Done);
    end Worker;
    P : Ints.Attribute_Handle;
begin
    if Current_State (Gate) then raise Program_Error; end if;
    P := Ints.Reference;
    P.all := 11;
    if Ints.Value /= 11 then raise Program_Error; end if;
    Ints.Set_Value (12);
    if P.all /= 12 or else Ints.Reference /= P then raise Program_Error; end if;
    Ints.Reinitialize;
    if Ints.Value /= 7 then raise Program_Error; end if;
    Set_True (Gate);
    Suspend_Until_True (Done);
    Put_Line ("task services OK");
end TaskServices;
