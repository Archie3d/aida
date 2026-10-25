with Ada.Text_IO;
with Interface_Model;
procedure InterfaceGenerics is
    generic
        Seed : Integer;
    package Factory is
        type Item is new Interface_Model.Reader and Interface_Model.Meter with record
            Data : Integer := Seed;
        end record;
        overriding function Read (Self : Item) return Integer;
        overriding function Measure (Self : Item) return Integer;
    end Factory;
    package body Factory is
        overriding function Read (Self : Item) return Integer is
        begin
            return Self.Data;
        end Read;
        overriding function Measure (Self : Item) return Integer is
        begin
            return Self.Data + Seed;
        end Measure;
    end Factory;
    procedure Run (Seed : Integer) is
        package First is new Factory (Seed);
        package Second is new Factory (Seed + 1);
        A : First.Item;
        B : Second.Item;
        R : Interface_Model.Reader'Class := A;
        S : Interface_Model.Reader'Class := B;
        M : Interface_Model.Meter'Class := Interface_Model.Meter'Class (R);
    begin
        if Interface_Model.Read (R) /= Seed or else Interface_Model.Read (S) /= Seed + 1
            or else Interface_Model.Measure (M) /= Seed * 2 then
            raise Program_Error;
        end if;
        Interface_Model.Exercise (R);
        Interface_Model.Exercise (S);
        S := Interface_Model.Reader'Class (B);
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (17);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("interface generic worker failed");
    end Worker;
begin
    Run (11);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("interface generics passed");
end InterfaceGenerics;
