with Ada.Text_IO;
with Ada.Task_Identification;
with IdentityLibrary;
procedure TaskIdentityObjects is
    use Ada.Task_Identification;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    generic
    package Workers is
        task type Worker;
        type Pair is array (1 .. 2) of Worker;
        type Holder is record
            Marker : Character := 'x';
            Object : Worker;
        end record;
        Objects : Pair;
        Record_Object : Holder;
        function Passed return Integer;
    end Workers;
    package body Workers is
        protected Counter is
            procedure Increment;
            function Value return Integer;
        private
            Count : Integer := 0;
        end Counter;
        protected body Counter is
            procedure Increment is
            begin
                Count := Count + 1;
            end Increment;
            function Value return Integer is
            begin
                return Count;
            end Value;
        end Counter;
        task body Worker is
        begin
            Check (Worker'Identity = Current_Task);
            Counter.Increment;
        end Worker;
        function Passed return Integer is
        begin
            return Counter.Value;
        end Passed;
    end Workers;
    package First is new Workers;
    package Second is new Workers;
    Library_Id : Task_Id;
begin
    IdentityLibrary.Results.Await_Result (Library_Id);
    Check (Library_Id = IdentityLibrary.Worker'Identity);
    Check (IdentityLibrary.Environment = Current_Task);
    Check (First.Objects (1)'Identity /= First.Objects (2)'Identity);
    Check (First.Objects (1)'Identity /= Second.Objects (1)'Identity);
    Check (First.Record_Object.Object'Identity /= First.Objects (1)'Identity);
    while not First.Objects (1)'Terminated or not First.Objects (2)'Terminated
        or not First.Record_Object.Object'Terminated or not Second.Objects (1)'Terminated
        or not Second.Objects (2)'Terminated or not Second.Record_Object.Object'Terminated loop
        delay 0.001;
    end loop;
    Check (First.Passed = 3 and Second.Passed = 3);
    Ada.Text_IO.Put_Line ("task identity objects passed");
end TaskIdentityObjects;
