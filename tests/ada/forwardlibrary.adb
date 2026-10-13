with Forward_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure Forwardlibrary is
    function Instance is new Forward_Model.Template (3);
begin
    if Forward_Model.Early /= 0 then
        raise Program_Error;
    end if;
    Put_Line (Integer'Image (Forward_Model.Invoke (2)));
    Put_Line (Integer'Image (Instance));
    Put_Line ("library forward instances ready");
end Forwardlibrary;
