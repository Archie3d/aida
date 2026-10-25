with Ada.Text_IO;
procedure TaggedViews is
    procedure Run is
        package Model is
            type Root (Length : Positive) is tagged record
                Value : Integer := Length;
            end record;
            function Read (Self : Root) return Integer;
            type Child is new Root with null record;
            type Base is tagged record
                Value : Integer := 4;
            end record;
            function Read (Self : Base) return Integer;
            type Part is new Base with private;
            type Opaque (<>) is private;
            function Make return Opaque;
            type Tagged_Opaque (<>) is tagged private;
            function Make_Tagged return Tagged_Opaque;
            function Value (Self : Tagged_Opaque) return Integer;
        private
            type Part is new Base with record
                Secret : Integer := 11;
            end record;
            type Opaque is record
                Value : Integer := 17;
            end record;
            type Tagged_Opaque is tagged record
                Data : Integer := 23;
            end record;
        end Model;
        package body Model is
            function Read (Self : Root) return Integer is
            begin
                return Self.Length + Self.Value;
            end Read;
            function Read (Self : Base) return Integer is
            begin
                return Self.Value;
            end Read;
            function Make_Tagged return Tagged_Opaque is
            begin
                return (Data => 23);
            end Make_Tagged;
            function Value (Self : Tagged_Opaque) return Integer is
            begin
                return Self.Data;
            end Value;
            function Make return Opaque is
            begin
                return (Value => 17);
            end Make;
        end Model;
        package Extensions is
            type Child is new Model.Part with record
                Extra : Integer := 8;
            end record;
            type Discriminated (Count : Positive) is new Model.Base with record
                Other : Integer := Count;
            end record;
            type Opaque_Child is new Model.Tagged_Opaque with null record;
            type Sized is tagged null record;
            for Sized'Size use 128;
            type Sized_Child is new Sized with record
                Extra : Integer := 23;
            end record;
            type Sized_Null is new Sized with null record;
        end Extensions;
        Source : Model.Tagged_Opaque := Model.Make_Tagged;
        Opaque_Child : Extensions.Opaque_Child := (Source with null record);
        Sized : Extensions.Sized_Child;
        Part : Model.Part;
        Item : Extensions.Child := (Part with Extra => 8);
        D : Extensions.Discriminated (7);
        R : Model.Root (3);
        C : Model.Child (5);
        Wide : Model.Root'Class := C;
        Opaque : Model.Opaque := Model.Make;
        type Node;
        type Link is access all Node;
        type Holder (Owner : access Node) is limited null record;
        type Node is limited record
            Value : Integer := 1;
            Self : Link := Node'Access;
            Part : Holder (Node'Access);
        end record;
        N : Node;
    begin
        N.Self.Value := 9;
        N.Part.Owner.Value := 12;
        if Extensions.Value (Opaque_Child) /= 23 or else N.Value /= 12 or else Model.Read (R) /= 6 or else Model.Read (Wide) /= 10
            or else Item.Value /= 4 or else Item.Extra /= 8 or else D.Count /= 7
            or else D.Other /= 7 or else Extensions.Sized'Size /= 128
            or else Extensions.Sized_Null'Size /= 128
            or else Extensions.Sized_Child'Size /= 192 or else Sized.Extra /= 23 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("tagged views worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("tagged views passed");
end TaggedViews;
