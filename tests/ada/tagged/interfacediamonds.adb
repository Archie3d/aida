with Ada.Text_IO;
with Ada.Tags;
procedure InterfaceDiamonds is
    use type Ada.Tags.Tag;
    procedure Run (Seed : Integer) is
        package Model is
            type Common is interface;
            function Value (Self : Common) return Integer is abstract;
            procedure Reset (Self : in out Common) is abstract;
            type Left is interface and Common;
            function Left_Value (Self : Left) return Integer is abstract;
            type Right is interface and Common;
            function Right_Value (Self : Right) return Integer is abstract;
            overriding procedure Reset (Self : in out Right) is null;
            type Diamond is interface and Left and Right;
            type Item is new Diamond with record
                Data : Integer := Seed;
            end record;
            overriding function Value (Self : Item) return Integer;
            overriding function Left_Value (Self : Item) return Integer;
            overriding function Right_Value (Self : Item) return Integer;
            type Child is new Item with null record;
            overriding function Value (Self : Child) return Integer;
        end Model;
        package body Model is
            overriding function Value (Self : Item) return Integer is
            begin
                return Self.Data;
            end Value;
            overriding function Left_Value (Self : Item) return Integer is
            begin
                return Self.Data + 1;
            end Left_Value;
            overriding function Right_Value (Self : Item) return Integer is
            begin
                return Self.Data + 2;
            end Right_Value;
            overriding function Value (Self : Child) return Integer is
            begin
                return Self.Data + 100;
            end Value;
        end Model;
        Object : Model.Child;
        D : Model.Diamond'Class := Object;
        C : Model.Common'Class := Model.Common'Class (D);
        L : Model.Left'Class := Model.Left'Class (C);
        R : Model.Right'Class := Model.Right'Class (L);
        Tags : Ada.Tags.Tag_Array := Ada.Tags.Interface_Ancestor_Tags (Object'Tag);
        Seen : Integer := 0;
    begin
        Model.Reset (C);
        if Model.Value (C) /= Seed + 100 or else Model.Left_Value (L) /= Seed + 1
            or else Model.Right_Value (R) /= Seed + 2 or else Tags'Length /= 4 then
            raise Program_Error;
        end if;
        for Index in Tags'Range loop
            if Tags (Index) = Model.Common'Tag then
                Seen := Seen + 1;
            end if;
        end loop;
        if Seen /= 1 or else not Ada.Tags.Is_Abstract (Model.Diamond'Tag)
            or else not Ada.Tags.Is_Descendant_At_Same_Level (Object'Tag, Model.Common'Tag) then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (17);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("interface diamond worker failed");
    end Worker;
begin
    Run (11);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("interface diamonds passed");
end InterfaceDiamonds;
