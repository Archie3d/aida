with Ada.Text_IO;
procedure SharedConcurrency is
    full : Boolean := False;
    pragma Atomic (full);
    payload : Long_Integer := 0;
    pragma Volatile (payload);
    ordinary : Integer := 0;
    failed : Boolean := False;
    pragma Atomic (failed);
begin
    declare
        task Producer;
        task body Producer is
            procedure Publish (value : Long_Integer) is
                alias : Long_Integer renames payload;
            begin
                alias := value;
            end Publish;
        begin
            for iteration in 1 .. 10000 loop
                while full loop
                    null;
                end loop;
                ordinary := iteration;
                Publish (Long_Integer (iteration) * 1000000000);
                full := True;
            end loop;
        end Producer;
        task Consumer;
        task body Consumer is
        begin
            for iteration in 1 .. 10000 loop
                while not full loop
                    null;
                end loop;
                if ordinary /= iteration or payload /= Long_Integer (iteration) * 1000000000 then
                    failed := True;
                end if;
                full := False;
            end loop;
        end Consumer;
    begin
        null;
    end;
    if failed or full or ordinary /= 10000 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("shared concurrency passed");
end SharedConcurrency;
