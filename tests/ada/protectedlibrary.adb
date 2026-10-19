with Ada.Text_IO;
with Protected_Model;
procedure ProtectedLibrary is
begin
    declare
        task type Worker;
        task body Worker is
        begin
            for iteration in 1 .. 1000 loop
                Protected_Model.shared.Add (2);
            end loop;
        end Worker;
        first, second : Worker;
    begin
        null;
    end;
    if Protected_Model.shared.Get /= 4000 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("protected library passed");
end ProtectedLibrary;
