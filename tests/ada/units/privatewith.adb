with Ada.Text_IO; use Ada.Text_IO;
with Context_Client;
with Context_Client.Public;
with Context_Impl;
procedure PrivateWith is
    Object : Context_Client.Data;
    Handle : Context_Impl.Handle := Context_Impl.Create;
begin
    if Context_Client.Read (Object) /= 34 or Context_Client.Nested.Read /= 17
        or Context_Client.Public.Read /= 34 or Context_Impl.Read (Handle) /= 23 then
        raise Program_Error;
    end if;
    Put_Line ("private with passed");
end PrivateWith;
