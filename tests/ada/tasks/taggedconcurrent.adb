with Ada.Finalization;
with Ada.Text_IO;
procedure TaggedConcurrent is
    protected Counts is
        procedure Finish;
        procedure Complete;
        function Completed return Natural;
        function Value return Natural;
    private
        total : Natural := 0;
        successful : Natural := 0;
    end Counts;
    protected body Counts is
        procedure Finish is
        begin
            total := total + 1;
        end Finish;
        procedure Complete is
        begin
            successful := successful + 1;
        end Complete;
        function Completed return Natural is
        begin
            return successful;
        end Completed;
        function Value return Natural is
        begin
            return total;
        end Value;
    end Counts;
    type Root is new Ada.Finalization.Limited_Controlled with record
        value : Integer := 17;
    end record;
    overriding procedure Finalize (object : in out Root) is
    begin
        Counts.Finish;
    end Finalize;
    protected type Cell is
        procedure Set (item : Integer);
        function Get return Integer;
        procedure Fail;
    private
        value : Integer := 0;
    end Cell;
    protected body Cell is
        procedure Set (item : Integer) is
        begin
            value := item;
        end Set;
        procedure Fail is
        begin
            raise Constraint_Error;
        end Fail;
        function Get return Integer is
        begin
            return value;
        end Get;
    end Cell;
    task type Echo is
        entry Exchange (value : in out Integer);
    end Echo;
    task body Echo is
    begin
        accept Exchange (value : in out Integer) do
            value := value + 1;
        end Exchange;
    end Echo;
    type Parent is new Root with record
        first : Cell;
        worker : Echo;
    end record;
    type Child is new Parent with record
        second : Cell;
        other : Echo;
    end record;
    procedure CheckTag (object : Root'Class) is
    begin
        if object not in Child then
            raise Program_Error;
        end if;
    end CheckTag;
    function Fail return Integer is
    begin
        raise Constraint_Error;
        return 0;
    end Fail;
    type Broken is new Child with record
        failure : Integer := Fail;
    end record;
    procedure Exercise (seed : Integer) is
        a : Child := (Parent with second => <>, other => <>);
        b : Child := (Parent'(Root with first => <>, worker => <>)
                      with second => <>, other => <>);
        c : Child;
        value : Integer := seed;
        procedure Check (object : in out Child; offset : Integer) is
        begin
            CheckTag (object);
            if object.value /= 17 then
                raise Program_Error;
            end if;
            begin
                object.first.Fail;
            exception
                when Constraint_Error => null;
            end;
            Parent (object).first.Set (seed + offset);
            object.second.Set (seed + offset + 1);
            object.worker.Exchange (value);
            object.other.Exchange (value);
            if object.first.Get /= seed + offset
                or else object.second.Get /= seed + offset + 1
            then
                raise Program_Error;
            end if;
        end Check;
    begin
        begin
            declare
                unfinished : Broken;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        Check (a, 0);
        Check (b, 2);
        Check (c, 4);
        if value /= seed + 6 or else a.first.Get /= seed
            or else b.first.Get /= seed + 2
        then
            raise Program_Error;
        end if;
    end Exercise;
    task type Runner;
    task body Runner is
    begin
        for i in 1 .. 5 loop
            Exercise (i * 10);
        end loop;
        Counts.Complete;
    end Runner;
begin
    declare
        runners : array (1 .. 4) of Runner;
    begin
        null;
    end;
    if Counts.Value /= 60 or else Counts.Completed /= 4 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("tagged concurrent OK");
end TaggedConcurrent;
