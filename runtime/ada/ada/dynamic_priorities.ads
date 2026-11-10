with System;
with Ada.Task_Identification;
package Ada.Dynamic_Priorities is
    pragma Preelaborate;
    procedure Set_Priority (Priority : in System.Any_Priority;
        T : in Ada.Task_Identification.Task_Id := Ada.Task_Identification.Current_Task);
    pragma Import (C, Set_Priority, "__ada_set_priority");
    function Get_Priority (T : Ada.Task_Identification.Task_Id := Ada.Task_Identification.Current_Task)
        return System.Any_Priority;
    pragma Import (C, Get_Priority, "__ada_get_priority");
end Ada.Dynamic_Priorities;
