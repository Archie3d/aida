with Ada.Text_IO;
procedure ProtectedConcurrency is
    protected type Counter is
        procedure Increment;
        procedure Fail;
        function Get return Integer;
    private
        value : Integer := 0;
    end Counter;
    protected body Counter is
        procedure Increment is
        begin
            value := value + 1;
        end Increment;
        procedure Fail is
        begin
            raise Constraint_Error;
        end Fail;
        function Get return Integer is
        begin
            return value;
        end Get;
    end Counter;
    shared : Counter;
begin
    for pass in 1 .. 3 loop
        declare
            task type Worker;
            task body Worker is
            begin
                for iteration in 1 .. 3000 loop
                    shared.Increment;
                    if shared.Get < 0 then
                        raise Program_Error;
                    end if;
                end loop;
                begin
                    shared.Fail;
                exception
                    when Constraint_Error => shared.Increment;
                end;
            end Worker;
            type Workers is array (1 .. 4) of Worker;
            workersObject : Workers;
        begin
            null;
        end;
        if shared.Get /= pass * 12004 then
            raise Program_Error with "protected counter lost an update";
        end if;
    end loop;
    Ada.Text_IO.Put_Line ("protected concurrency passed");
end ProtectedConcurrency;
