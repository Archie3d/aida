with Ada.Text_IO;
procedure InterruptBounds is
    failures : Natural := 0;
    protected type Lock (ceiling : Integer) is
        pragma Interrupt_Priority (ceiling);
        procedure Touch;
    end Lock;
    protected body Lock is
        procedure Touch is
        begin
            null;
        end Touch;
    end Lock;
    protected Ordinary is
        pragma Priority (31);
        procedure Touch;
    end Ordinary;
    protected body Ordinary is
        procedure Touch is
        begin
            null;
        end Touch;
    end Ordinary;
begin
    begin
        declare
            object : Lock (30);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        declare
            object : Lock (32);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        declare
            task Bad is
                pragma Interrupt_Priority (32);
            end Bad;
            task body Bad is
            begin
                null;
            end Bad;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        declare
            task Bad is
                pragma Priority (31);
            end Bad;
            task body Bad is
            begin
                null;
            end Bad;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    begin
        declare
            task Bad is
                pragma Interrupt_Priority (-1);
            end Bad;
            task body Bad is
            begin
                null;
            end Bad;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => failures := failures + 1;
    end;
    if failures /= 5 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("interrupt priority bounds OK");
end InterruptBounds;
