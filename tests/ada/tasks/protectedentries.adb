with Ada.Text_IO;
with Ada.Exceptions;
procedure ProtectedEntries is
    protected type Gate is
        entry Join (id : Integer := 1);
        entry SecondStart;
        entry Text (value : String; length : out Integer);
        procedure StartSecond;
        procedure Open;
        function Queued return Natural;
        function Result return Integer;
    private
        ready : Boolean := False;
        secondReady : Boolean := False;
        total : Integer := 0;
    end Gate;
    protected body Gate is
        entry Join (id : Integer := 1) when ready is
            procedure Add is
            begin
                total := total * 10 + id;
            end Add;
        begin
            Add;
            if id = 1 then
                raise Constraint_Error with "queued entry body";
            end if;
        end Join;
        entry SecondStart when secondReady is
        begin
            null;
        end SecondStart;
        entry Text (value : String; length : out Integer) when True is
        begin
            length := value'Length;
        end Text;
        procedure StartSecond is
        begin
            secondReady := True;
        end StartSecond;
        procedure Open is
        begin
            ready := True;
        end Open;
        function Queued return Natural is
        begin
            return Join'Count;
        end Queued;
        function Result return Integer is
        begin
            return total;
        end Result;
    end Gate;
    protected type Failure is
        entry Wait;
        entry Other;
        procedure BreakBarrier;
        function Queued return Natural;
    private
        broken : Boolean := False;
        zero : Integer := 0;
    end Failure;
    protected body Failure is
        entry Wait when broken and then 1 / zero = 0 is
        begin
            raise Constraint_Error;
        end Wait;
        entry Other when False is
        begin
            null;
        end Other;
        procedure BreakBarrier is
        begin
            broken := True;
            raise Constraint_Error with "opener preserved";
        end BreakBarrier;
        function Queued return Natural is
        begin
            return Wait'Count + Other'Count;
        end Queued;
    end Failure;
    c : Gate;
    f : Failure;
    length : Integer := 0;
begin
    declare
        task First;
        task Second;
        task body First is
            caught : Boolean := False;
        begin
            begin
                c.Join;
            exception
                when e : Constraint_Error =>
                    if Ada.Exceptions.Exception_Message (e) = "queued entry body" then
                        caught := True;
                    end if;
            end;
            if not caught then
                c.Join (9);
            end if;
        end First;
        task body Second is
        begin
            c.SecondStart;
            c.Join (id => 2);
        end Second;
    begin
        while c.Queued /= 1 loop
            null;
        end loop;
        c.StartSecond;
        while c.Queued /= 2 loop
            null;
        end loop;
        c.Open;
        -- The opener returns after all eligible queued entry bodies finish.
        if c.Result /= 12 or c.Queued /= 0 then
            raise Program_Error;
        end if;
    end;
    c.Text (length => length, value => "queued ABI");
    if length /= 10 then
        raise Program_Error;
    end if;
    declare
        task First;
        task Second;
        task body First is
        begin
            f.Wait;
            raise Constraint_Error with "barrier failed to raise";
        exception
            when Program_Error => c.Join (3);
        end First;
        task body Second is
        begin
            f.Other;
            raise Constraint_Error with "barrier failed to raise";
        exception
            when Program_Error => c.Join (4);
        end Second;
    begin
        while f.Queued /= 2 loop
            null;
        end loop;
        begin
            f.BreakBarrier;
            raise Program_Error;
        exception
            when e : Constraint_Error =>
                if Ada.Exceptions.Exception_Message (e) /= "opener preserved" then
                    raise Program_Error;
                end if;
        end;
    end;
    if c.Result /= 1234 and c.Result /= 1243 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("protected entries passed");
end ProtectedEntries;
