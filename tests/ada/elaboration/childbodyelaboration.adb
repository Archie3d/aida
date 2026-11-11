with Ada.Text_IO;
package Parent_View is
    function Value return Integer;
end Parent_View;
package Parent_View.Child is
    Result : Integer := 0;
end Parent_View.Child;
package body Parent_View.Child is
begin
    Result := Parent_View.Value;
end Parent_View.Child;
package body Parent_View is
    function Value return Integer is
    begin
        return 42;
    end Value;
end Parent_View;
package Fallback_Parent is
    function Value return Integer;
end Fallback_Parent;
package Fallback_Parent.Child is
    pragma Elaborate_Body;
    Result : Integer := 0;
end Fallback_Parent.Child;
package body Fallback_Parent.Child is
begin
    Result := 7;
end Fallback_Parent.Child;
with Fallback_Parent.Child;
package body Fallback_Parent is
    function Value return Integer is
    begin
        return Child.Result + 35;
    end Value;
end Fallback_Parent;
with Ada.Text_IO;
with Parent_View.Child;
with Fallback_Parent;
procedure Childbodyelaboration is
begin
    if Parent_View.Child.Result /= 42 or else Fallback_Parent.Value /= 42 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("child body elaboration passed");
end Childbodyelaboration;
