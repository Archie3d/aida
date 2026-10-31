with Ada.Text_IO;
with Ada.Tags;
procedure InterfaceCategories is
    use type Ada.Tags.Tag;
    procedure Run (Seed : Integer) is
        package Model is
            type Counter is limited interface;
            procedure Increment (Self : in out Counter) is abstract;
            function Value (Self : Counter) return Integer is abstract;
            type Synchronized_Counter is synchronized interface and Counter;
            type Task_Counter is task interface and Synchronized_Counter;
            type Protected_Counter is protected interface and Synchronized_Counter;
            type Item is limited new Counter with record
                Data : Integer := Seed;
            end record;
            overriding procedure Increment (Self : in out Item);
            overriding function Value (Self : Item) return Integer;
            type Copyable is new Counter with record
                Data : Integer := 3;
            end record;
            overriding procedure Increment (Self : in out Copyable);
            overriding function Value (Self : Copyable) return Integer;
        end Model;
        package body Model is
            overriding procedure Increment (Self : in out Item) is
            begin
                Self.Data := Self.Data + 1;
            end Increment;
            overriding function Value (Self : Item) return Integer is
            begin
                return Self.Data;
            end Value;
            overriding procedure Increment (Self : in out Copyable) is
            begin
                Self.Data := Self.Data + 1;
            end Increment;
            overriding function Value (Self : Copyable) return Integer is
            begin
                return Self.Data;
            end Value;
        end Model;
        Object : aliased Model.Item;
        A, B : Model.Copyable;
        type Link is access all Model.Counter'Class;
        Reference : Link := Object'Access;
        Tags : Ada.Tags.Tag_Array := Ada.Tags.Interface_Ancestor_Tags (Model.Task_Counter'Tag);
    begin
        B := A;
        Model.Increment (Reference.all);
        if Model.Value (Reference.all) /= Seed + 1 or else Model.Value (B) /= 3
            or else Tags'Length /= 2 or else Ada.Tags.Parent_Tag (Model.Task_Counter'Tag) /= Ada.Tags.No_Tag then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (17);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("interface category worker failed");
    end Worker;
begin
    Run (11);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("interface categories passed");
end InterfaceCategories;
