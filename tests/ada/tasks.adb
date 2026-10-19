with Ada.Text_IO;
with Ada.Exceptions;
procedure Tasks is
    procedure Check (condition : Boolean) is
    begin
        if not condition then
            raise Program_Error with "task check failed";
        end if;
    end Check;
    value : Integer := 0;
    package HiddenTasks is
        type Hidden is limited private;
        subtype AliasHidden is Hidden;
    private
        task type Hidden;
    end HiddenTasks;
    package body HiddenTasks is
        task body Hidden is
        begin
            null;
        end Hidden;
    end HiddenTasks;
    hiddenObject : HiddenTasks.AliasHidden;
    procedure LocalTasks is
        task Worker;
        task body Worker is
            initial : Integer := 41;
        begin
            value := initial + 1;
        end Worker;
    begin
        null;
    end LocalTasks;
    procedure NestedTasks is
        task OuterWorker;
        task body OuterWorker is
            task InnerWorker;
            task body InnerWorker is
            begin
                value := 17;
            end InnerWorker;
        begin
            null;
        end OuterWorker;
    begin
        null;
    end NestedTasks;
    function Fail return Integer is
    begin
        raise Constraint_Error with "activation failure";
        return 0;
    end Fail;
begin
    LocalTasks;
    Check (value = 42);
    NestedTasks;
    Check (value = 17);
    value := 42;
    for iteration in 1 .. 3 loop
        declare
            task type Worker;
            task body Worker is
            begin
                null;
            end Worker;
            type Group is array (1 .. 3) of Worker;
            type Holder is limited record
                member : Worker;
                members : Group;
            end record;
            object : Holder;
            aliasObject : Holder renames object;
            procedure Observe (item : in out Holder) is
            begin
                null;
            end Observe;
        begin
            Observe (aliasObject);
        end;
    end loop;
    -- Task activation failure is outside the block's handled statements.
    begin
        declare
            task Broken;
            task body Broken is
                initial : Integer := Fail;
            begin
                raise Program_Error;
            exception
                when others => raise Program_Error;
            end Broken;
        begin
            raise Program_Error;
        exception
            when Tasking_Error => raise Program_Error;
        end;
        raise Program_Error;
    exception
        when Tasking_Error => null;
    end;
    -- An exception escaping the statements completes only the failing task.
    declare
        task Broken;
        task body Broken is
        begin
            raise Constraint_Error with "body failure";
        end Broken;
    begin
        null;
    end;
    -- Declaration failure before activation prevents task execution.
    begin
        declare
            task NeverStarted;
            task body NeverStarted is
            begin
                value := -1;
            end NeverStarted;
            initial : Integer := Fail;
        begin
            null;
        end;
    exception
        when Constraint_Error => null;
    end;
    Check (value = 42);
    -- Both specification and body objects activate before package statements.
    declare
        package Local is
            task First;
        end Local;
        package body Local is
            task body First is
            begin
                null;
            end First;
            task Second;
            task body Second is
            begin
                null;
            end Second;
        begin
            null;
        end Local;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("tasks passed");
end Tasks;
