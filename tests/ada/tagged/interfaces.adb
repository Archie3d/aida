with Ada.Text_IO;
with Ada.Tags;
with Ada.Unchecked_Deallocation;
procedure Interfaces is
    procedure Run (Seed : Integer) is
        package Model is
            type Readable is interface;
            function Read (Self : Readable) return Integer is abstract;
            procedure Touch (Self : in out Readable) is null;
            type Metrics is interface;
            function Measure (Self : Metrics) return Integer is abstract;
            type Both is interface and Readable and Metrics;
            type Base is tagged record
                Data : Integer := Seed;
            end record;
            function Read (Self : Base) return Integer;
            type Item is new Base and Both with record
                Extra : Integer := 7;
            end record;
            overriding function Measure (Self : Item) return Integer;
            type Child is new Item with null record;
            overriding function Read (Self : Child) return Integer;
            type Solo is new Readable with record
                Data : Integer := 3;
            end record;
            overriding function Read (Self : Solo) return Integer;
        end Model;
        package body Model is
            function Read (Self : Base) return Integer is
            begin
                return Self.Data;
            end Read;
            overriding function Measure (Self : Item) return Integer is
            begin
                return Self.Data + Self.Extra;
            end Measure;
            overriding function Read (Self : Child) return Integer is
            begin
                return Self.Data + 100;
            end Read;
            overriding function Read (Self : Solo) return Integer is
            begin
                return Self.Data;
            end Read;
        end Model;
        A : aliased Model.Child;
        Other : aliased Model.Solo;
        R : Model.Readable'Class := A;
        M : Model.Metrics'Class := Model.Metrics'Class (R);
        Base_View : Model.Base'Class := Model.Base'Class (R);
        type Reader is access all Model.Readable'Class;
        type Meter is access all Model.Metrics'Class;
        Pointer : Reader := A'Access;
        Cross : Meter := Meter (Pointer);
        type Owner is access Model.Readable'Class;
        Allocated : Owner := new Model.Child;
        procedure Free is new Ada.Unchecked_Deallocation (Model.Readable'Class, Owner);
        Ancestors : Ada.Tags.Tag_Array := Ada.Tags.Interface_Ancestor_Tags (A'Tag);
        Rejected : Boolean := False;
    begin
        if Model.Read (Base_View) /= Seed + 100 or else Model.Read (R) /= Seed + 100 or else Model.Measure (M) /= Seed + 7
            or else Model.Read (Pointer.all) /= Seed + 100
            or else Model.Measure (Cross.all) /= Seed + 7
            or else Model.Read (Allocated.all) /= Seed + 100 then
            raise Program_Error;
        end if;
        Model.Touch (R);
        if not (R in Model.Metrics'Class) or else not (M in Model.Readable'Class)
            or else R in Model.Solo'Class or else Ancestors'Length /= 3 then
            raise Program_Error;
        end if;
        begin
            declare
                Wrong : Model.Metrics'Class := Model.Metrics'Class (Model.Readable'Class (Other));
            begin
                null;
            end;
        exception
            when Constraint_Error => Rejected := True;
        end;
        if not Rejected then
            raise Program_Error;
        end if;
        Pointer := Other'Access;
        Rejected := False;
        begin
            Cross := Meter (Pointer);
        exception
            when Constraint_Error => Rejected := True;
        end;
        if not Rejected then
            raise Program_Error;
        end if;
        Pointer := null;
        Cross := Meter (Pointer);
        if Cross /= null then
            raise Program_Error;
        end if;
        Free (Allocated);
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (17);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("interface worker failed");
    end Worker;
begin
    Run (11);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("interfaces passed");
end Interfaces;
