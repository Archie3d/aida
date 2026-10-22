with Ada.Text_IO;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure AbortDeferral is
    function Depth return Integer;
    pragma Import (C, Depth, "__test_abort_depth");
    type Guard is new Ada.Finalization.Controlled with record
        Fail : Boolean := False;
    end record;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        if Depth < 1 then
            raise Storage_Error;
        end if;
        if Object.Fail then
            raise Constraint_Error;
        end if;
    end Finalize;
    protected Results is
        procedure Run;
        procedure Passed;
        procedure Failed;
        function Successes return Integer;
        function Failures return Integer;
    private
        Good : Integer := 0;
        Bad : Integer := 0;
    end Results;
    protected body Results is
        procedure Run is
            Object : Guard;
        begin
            if Depth /= 1 then
                raise Program_Error;
            end if;
            Object.Fail := True;
            return;
        end Run;
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        procedure Failed is
        begin
            Bad := Bad + 1;
        end Failed;
        function Successes return Integer is
        begin
            return Good;
        end Successes;
        function Failures return Integer is
        begin
            return Bad;
        end Failures;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
            type Pointer is access Guard;
            procedure Free is new Ada.Unchecked_Deallocation (Guard, Pointer);
            Object : Pointer;
        begin
            for I in 1 .. 20 loop
                if Depth /= 0 then
                    raise Program_Error;
                end if;
                begin
                    declare
                        Local : Guard;
                    begin
                        raise Constraint_Error;
                    end;
                exception
                    when Constraint_Error => null;
                end;
                begin
                    Results.Run;
                    raise Constraint_Error;
                exception
                    when Program_Error => null;
                end;
                if Depth /= 0 then
                    raise Program_Error;
                end if;
                Object := new Guard;
                Free (Object);
            end loop;
            Results.Passed;
        exception
            when others => Results.Failed;
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
begin
    Exercise;
    if Depth /= 0 or else Results.Successes /= 4 or else Results.Failures /= 0 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("abort deferral task contexts");
end AbortDeferral;
