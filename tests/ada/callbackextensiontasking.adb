with Ada.Text_IO; use Ada.Text_IO;
with Callback_Import_Provider;
procedure CallbackExtensionTasking is
    protected Results is
        procedure Finished;
        function Count return Integer;
    private
        Total : Integer := 0;
    end Results;
    protected body Results is
        procedure Finished is
        begin
            Total := Total + 1;
        end Finished;
        function Count return Integer is
        begin
            return Total;
        end Count;
    end Results;
    task type Worker;
    task body Worker is
        Bias : Integer := 10;
        package Parent is
            type Root is tagged record
                Value : Integer := 32;
            end record;
            function Read (Object : Root) return Integer;
        end Parent;
        package body Parent is
            function Read (Object : Root) return Integer is
            begin
                return Object.Value + Bias;
            end Read;
        end Parent;
        package Child is
            type Derived is new Parent.Root with null record;
        end Child;
        type Reader is access function (Object : Child.Derived) return Integer;
        F : Reader := Child.Read'Access;
        Object : Child.Derived;
        function Absolute (Value : Integer) return Integer;
        pragma Import (C, Absolute, "abs");
        type C_Function is access function (Value : Integer) return Integer;
        pragma Convention (C, C_Function);
        C : C_Function := Absolute'Access;
        function Increment (Value : Integer) return Integer;
        pragma Import (Ada, Increment, "callback_import_provider__increment");
        type Ada_Function is access function (Value : Integer) return Integer;
        A : Ada_Function := Increment'Access;
    begin
        for Iteration in 1 .. 30 loop
            if F (Object) /= 42 or C (-42) /= 42 or A (41) /= 42 then raise Program_Error; end if;
        end loop;
        Results.Finished;
    end Worker;
    type Workers is array (1 .. 4) of Worker;
begin
    declare
        Tasks : Workers;
    begin
        null;
    end;
    if Results.Count /= 4 then raise Program_Error; end if;
    Put_Line ("task callback extensions passed");
end CallbackExtensionTasking;
