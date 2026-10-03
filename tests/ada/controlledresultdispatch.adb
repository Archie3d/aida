with Controlled_Result_Model; use Controlled_Result_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure ControlledResultDispatch is
    type Plain_Result is tagged null record;
    package Model is
        type Root is tagged null record;
        function Clone (Object : Root) return Root;
        type Child is new Root with record
            Item : Guard;
        end record;
        overriding function Clone (Object : Child) return Child;
        type Managed is new Guard with null record;
        function Clone (Object : Managed) return Managed;
        function Plain (Object : Managed) return Plain_Result;
        type Derived is new Managed with null record;
        overriding function Clone (Object : Derived) return Derived;
    end Model;
    package body Model is
        function Clone (Object : Root) return Root is
        begin
            return Object;
        end Clone;
        function Clone (Object : Child) return Child is
        begin
            return Object;
        end Clone;
        function Clone (Object : Managed) return Managed is
        begin
            return Object;
        end Clone;
        function Plain (Object : Managed) return Plain_Result is
            Local : Plain_Result;
        begin
            return Local;
        end Plain;
        function Clone (Object : Derived) return Derived is
        begin
            return Object;
        end Clone;
    end Model;
    use Model;
    procedure Borrow (Object : Root'Class) is
    begin
        null;
    end Borrow;
    procedure Hidden (Object : Root'Class) is
    begin
        Borrow (Clone (Object));
        Put_Line ("hidden result owned");
    end Hidden;
    procedure Borrow (Object : Managed'Class) is
    begin
        Check (Object.Data.Value = 42);
    end Borrow;
    procedure Dispatch (Object : Managed'Class) is
        Local : Plain_Result := Plain (Object);
    begin
        Borrow (Clone (Object));
    end Dispatch;
begin
    declare
        A : Child;
        B : Child := Clone (A);
        C : Derived;
    begin
        C.Data.Value := 42;
        Check (Objects = 3 and Resources = 2);
        Hidden (A);
        Dispatch (C);
        Check (Objects = 3 and Resources = 2);
    end;
    Check (Objects = 0 and Resources = 0);
    Put_Line ("controlled result dispatch passed");
end ControlledResultDispatch;
