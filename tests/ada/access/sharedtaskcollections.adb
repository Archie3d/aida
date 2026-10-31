with Ada.Text_IO;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure SharedTaskCollections is
    protected Counts is
        procedure Completed;
        procedure Failed;
        function Done return Integer;
        function Errors return Integer;
    private
        Finished : Integer := 0;
        Failures : Integer := 0;
    end Counts;
    protected body Counts is
        procedure Completed is
        begin
            Finished := Finished + 1;
        end Completed;
        procedure Failed is
        begin
            Failures := Failures + 1;
        end Failed;
        function Done return Integer is
        begin
            return Finished;
        end Done;
        function Errors return Integer is
        begin
            return Failures;
        end Errors;
    end Counts;
    procedure Run is
        task type Service;
        task body Service is
        begin
            Counts.Completed;
        end Service;
        type Pointer is access Service;
        type Derived_Pointer is new Pointer;
        procedure Release is new Ada.Unchecked_Deallocation (Service, Derived_Pointer);
        task type Allocator;
        task body Allocator is
            Object : Pointer;
            Derived : Derived_Pointer;
        begin
            for I in 1 .. 20 loop
                Object := new Service;
                if I mod 2 = 0 then
                    Derived := Derived_Pointer (Object);
                    Release (Derived);
                end if;
            end loop;
        exception
            when others => Counts.Failed;
        end Allocator;
        type Sentinel is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Finalize (Object : in out Sentinel) is
        begin
            -- Even objects declared after the access type finalize only after
            -- all tasks allocated by its dependent tasks have terminated.
            if Counts.Done /= 80 or else Counts.Errors /= 0 then
                raise Program_Error;
            end if;
        end Finalize;
        Check : Sentinel;
        type Group is array (1 .. 4) of Allocator;
        Workers : Group;
    begin
        null;
    end Run;
begin
    Run;
    if Counts.Done /= 80 or else Counts.Errors /= 0 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("shared task collection masters");
end SharedTaskCollections;
