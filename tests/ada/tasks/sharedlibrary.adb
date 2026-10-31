with Ada.Text_IO;
with Shared_Model;
procedure SharedLibrary is
    use type Shared_Model.Flag;
begin
    declare
        task Worker;
        task body Worker is
        begin
            Shared_Model.value := 123;
            Shared_Model.ready := Shared_Model.True;
        end Worker;
    begin
        while not Shared_Model.ready loop
            null;
        end loop;
        if Shared_Model.value /= 123 then
            raise Program_Error;
        end if;
    end;
    Ada.Text_IO.Put_Line ("shared library passed");
end SharedLibrary;
