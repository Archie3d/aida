with Ada.Text_IO;
with Entry_Model;
procedure EntryLibrary is
    task Producer;
    task body Producer is
    begin
        for i in 1 .. 1000 loop
            Entry_Model.shared.Put (i);
        end loop;
    end Producer;
    value : Integer;
    sum : Integer := 0;
begin
    for i in 1 .. 1000 loop
        Entry_Model.shared.Get (value);
        sum := sum + value;
    end loop;
    if sum /= 500500 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("library entries passed");
end EntryLibrary;
