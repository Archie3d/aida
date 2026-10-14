with Ada.Text_IO;
with Ep_Client;
with Ep_Parent.Child;
use Ep_Parent;
pragma Elaborate_All (Ep_Parent, Child);
with Ep_Template;
with Ep_Function;
with Ep_Instance;
procedure Ep_Main is
    package Instance is new Ep_Template (23);
begin
    if Ep_Parent.Child.Value /= 17 or Instance.Read /= 23 or Ep_Function /= 19
        or Ep_Instance.Read /= 29 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line (Integer'Image (Ep_Client.Value));
end Ep_Main;
