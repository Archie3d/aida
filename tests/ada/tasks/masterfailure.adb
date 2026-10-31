with Master_Model; use Master_Model;
procedure Masterfailure is
    Bad : Integer := Fail;
begin
    raise Program_Error;
end Masterfailure;
