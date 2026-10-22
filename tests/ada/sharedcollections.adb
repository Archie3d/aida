with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Shared_Collection_Model; use Shared_Collection_Model;
procedure SharedCollections is
    protected Channel is
        entry Put (Object : Link);
        entry Take (Object : out Link);
    private
        Stored : Link;
    end Channel;
    protected body Channel is
        entry Put (Object : Link) when Stored = null is
        begin
            Stored := Object;
        end Put;
        entry Take (Object : out Link) when Stored /= null is
        begin
            Object := Stored;
            Stored := null;
        end Take;
    end Channel;
    procedure Exchange is
        task type Producer;
        task type Consumer;
        task body Producer is
            Object : Link;
        begin
            for I in 1 .. 40 loop
                Object := new Guard;
                Object.Value := I;
                Channel.Put (Object);
            end loop;
            Counts.Completed;
        exception
            when others => Counts.Failed;
        end Producer;
        task body Consumer is
            Object : Link;
            Derived : Derived_Link;
            Replacement : Guard;
        begin
            for I in 1 .. 40 loop
                Channel.Take (Object);
                if Object.Value < 1 or else Object.Value > 40 then
                    raise Program_Error;
                end if;
                -- Assignment and finalization locate heap records registered
                -- by another task. Derived types use the same collection.
                Replacement.Value := 100;
                Object.all := Replacement;
                Derived := Derived_Link (Object);
                Free_Derived (Derived);
                if Derived /= null then
                    raise Program_Error;
                end if;
            end loop;
            Counts.Completed;
        exception
            when others => Counts.Failed;
        end Consumer;
        type Producers is array (1 .. 4) of Producer;
        type Consumers is array (1 .. 4) of Consumer;
        Senders : Producers;
        Receivers : Consumers;
    begin
        null;
    end Exchange;
    procedure Local_Collection is
        type Pointer is access Guard;
        procedure Release is new Ada.Unchecked_Deallocation (Guard, Pointer);
        function Fail return Integer is
        begin
            raise Constraint_Error;
            return 0;
        end Fail;
        type Partial is record
            First : Guard;
            Last : Integer := Fail;
        end record;
        type Partial_Pointer is access Partial;
        task type Worker;
        task body Worker is
            Object : Pointer;
            Broken : Partial_Pointer;
        begin
            for I in 1 .. 20 loop
                Object := new Guard;
                Release (Object);
                begin
                    Broken := new Partial;
                    raise Program_Error;
                exception
                    when Constraint_Error => null;
                end;
            end loop;
            -- The worker's context exits with this allocation still alive.
            Object := new Guard;
            Counts.Completed;
        exception
            when others => Counts.Failed;
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Local_Collection;
begin
    Exchange;
    if Counts.Live /= 0 or else Counts.Done /= 8 or else Counts.Errors /= 0 then
        raise Program_Error;
    end if;
    Local_Collection;
    if Counts.Live /= 0 or else Counts.Done /= 12 or else Counts.Errors /= 0 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("shared collection ownership");
end SharedCollections;
