with Ada.Task_Identification; use Ada.Task_Identification;
generic
    type Attribute is private;
    Initial_Value : in Attribute;
package Ada.Task_Attributes is
    type Attribute_Handle is access all Attribute;
    function Value (T : Task_Id := Current_Task) return Attribute;
    function Reference (T : Task_Id := Current_Task) return Attribute_Handle;
    procedure Set_Value (Val : in Attribute; T : in Task_Id := Current_Task);
    procedure Reinitialize (T : in Task_Id := Current_Task);
end Ada.Task_Attributes;
