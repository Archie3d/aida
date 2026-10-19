with Ada.Text_IO;
with Ada.Exceptions;
with Ada.Finalization;
procedure TaskConcurrency is
    procedure Reset;
    pragma Import (C, Reset, "taskReset");
    function Activate return Integer;
    pragma Import (C, Activate, "taskActivate");
    function ActivationCount return Integer;
    pragma Import (C, ActivationCount, "taskActivationCount");
    procedure ReleaseTasks;
    pragma Import (C, ReleaseTasks, "taskRelease");
    procedure Rendezvous;
    pragma Import (C, Rendezvous, "taskRendezvous");
    procedure Finalized;
    pragma Import (C, Finalized, "taskFinalized");
    package Model is
        type Guard is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Finalize (object : in out Guard);
    end Model;
    package body Model is
        overriding procedure Finalize (object : in out Guard) is
        begin
            Finalized;
        end Finalize;
    end Model;
    procedure Exercise (fail : Boolean) is
        guard : Model.Guard;
        task type Worker;
        task body Worker is
            initial : Integer := Activate;
        begin
            Rendezvous;
        end Worker;
        first, second : Worker;
    begin
        ReleaseTasks;
        if fail then
            raise Constraint_Error with "parent failure";
        end if;
    end Exercise;
    procedure ExerciseAllocated is
        task type Worker;
        task body Worker is
            initial : Integer := Activate;
        begin
            Rendezvous;
        end Worker;
        type Pair is record
            marker : Character := 'A';
            first, second : Worker;
        end record;
        type Pointer is access Pair;
        guard : Model.Guard;
        procedure Allocate is
            object : Pointer := new Pair;
        begin
            null;
        end Allocate;
    begin
        Allocate;
        ReleaseTasks;
    end ExerciseAllocated;
    procedure ExerciseNestedAllocated is
        task type Worker;
        task body Worker is
            initial : Integer := Activate;
        begin
            Rendezvous;
        end Worker;
        type Holder;
        type Pointer is access Holder;
        depth : Integer := 0;
        function MakeChild return Pointer;
        type Holder is record
            worker : Worker;
            child : Pointer := MakeChild;
        end record;
        function MakeChild return Pointer is
        begin
            depth := depth + 1;
            if depth = 1 then
                declare
                    object : Pointer := new Holder;
                begin
                    if ActivationCount /= 1 then
                        raise Program_Error with "outer allocation activated early";
                    end if;
                    return object;
                end;
            end if;
            return null;
        end MakeChild;
        guard : Model.Guard;
        object : Pointer := new Holder;
    begin
        ReleaseTasks;
    end ExerciseNestedAllocated;
begin
    for iteration in 1 .. 3 loop
        Reset;
        ExerciseNestedAllocated;
        Reset;
        ExerciseAllocated;
        Reset;
        Exercise (False);
        Reset;
        begin
            Exercise (True);
            raise Program_Error;
        exception
            when occurrence : Constraint_Error =>
                if Ada.Exceptions.Exception_Message (occurrence) /= "parent failure" then
                    raise Program_Error;
                end if;
        end;
        Reset;
        declare
            guard : Model.Guard;
            generic
                initial : Integer;
            package Workers is
                task type Worker;
                first, second : Worker;
            end Workers;
            package body Workers is
                task body Worker is
                    activation : Integer := Activate;
                begin
                    if initial /= 7 then
                        raise Program_Error;
                    end if;
                    Rendezvous;
                end Worker;
            begin
                ReleaseTasks;
            end Workers;
            package Instance is new Workers (7);
        begin
            null;
        end;
        -- Package tasks activate at the package body's begin, while their
        -- enclosing block remains in its declarative part.
        Reset;
        declare
            guard : Model.Guard;
            package Local is
                task First;
            end Local;
            package body Local is
                task body First is
                    initial : Integer := Activate;
                begin
                    Rendezvous;
                end First;
                task Second;
                task body Second is
                    initial : Integer := Activate;
                begin
                    Rendezvous;
                end Second;
            begin
                ReleaseTasks;
            end Local;
        begin
            null;
        end;
    end loop;
    Ada.Text_IO.Put_Line ("task concurrency passed");
end TaskConcurrency;
