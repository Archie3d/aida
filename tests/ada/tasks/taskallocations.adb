with Ada.Text_IO;
procedure TaskAllocations is
    value : Integer := 0;
    procedure EarlyAllocation is
        task type Worker;
        type Pointer is access Worker;
        function Attempt return Integer is
            object : Pointer;
        begin
            object := new Worker;
            return -1;
        exception
            when Program_Error => return 0;
        end Attempt;
        early : Integer := Attempt;
        task body Worker is
        begin
            value := 7;
        end Worker;
    begin
        if early /= 0 or value /= 0 then
            raise Program_Error;
        end if;
        if Attempt /= -1 then
            raise Program_Error;
        end if;
    end EarlyAllocation;
    procedure Exercise is
        task type Worker;
        task body Worker is
        begin
            value := 12;
        end Worker;
        type Holder is record
            marker : Character := 'A';
            member : Worker;
        end record;
        type Pointer is access Holder;
        procedure Allocate is
            object : Pointer := new Holder;
        begin
            null;
        end Allocate;
    begin
        Allocate;
        -- The task depends on Pointer's collection, which remains alive here.
        null;
    end Exercise;
begin
    EarlyAllocation;
    if value /= 7 then
        raise Program_Error;
    end if;
    Exercise;
    if value /= 12 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("task allocations passed");
end TaskAllocations;
