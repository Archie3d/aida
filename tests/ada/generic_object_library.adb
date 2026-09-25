with Ada.Text_IO;
package Object_State is
    Value : Integer := 6;
    Calls : Integer := 0;
    function Next_Value return Integer;
end Object_State;
package body Object_State is
    function Next_Value return Integer is
    begin
        Calls := Calls + 1;
        return Value;
    end Next_Value;
end Object_State;
generic
    Initial : Integer := Object_State.Next_Value;
    Target : in out Integer;
package Object_Template is
    Saved : Integer := Initial;
    procedure Add;
end Object_Template;
package body Object_Template is
    procedure Add is
    begin
        Target := Target + Initial;
    end Add;
begin
    Target := Target + 1;
end Object_Template;
package Object_First is new Object_Template (Target => Object_State.Value);
package Object_Second is new Object_Template (Target => Object_State.Value);
procedure Generic_Object_Library is
begin
    if Object_State.Calls /= 2 or Object_State.Value /= 8
        or Object_First.Saved /= 6 or Object_Second.Saved /= 7 then
        raise Program_Error;
    end if;
    Object_First.Add;
    Object_Second.Add;
    if Object_State.Value /= 21 or Object_State.Calls /= 2 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("library generic object elaboration and shared references");
end Generic_Object_Library;
