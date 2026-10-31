with Ada.Text_IO;
with Access_Result_Base;
procedure ControllingAccessResults is
    Saved : access Access_Result_Base.Root'Class;
    procedure Run is
        Factories : Natural := 0;
        package Model is
            type Leaf is new Access_Result_Base.Root with record
                Extra : Integer := 9;
            end record;
            overriding function Make (Mode : Integer := 0) return access Leaf;
            overriding function Identity (Self : access Leaf; Mode : Integer := 0) return access Leaf;
            overriding function Alias_Of (Self : access Leaf; Mode : Integer := 0) return access Leaf;
            overriding function Read (Self : Leaf) return Integer;
            type Other is new Access_Result_Base.Root with null record;
            overriding function Make (Mode : Integer := 0) return access Other;
            overriding function Identity (Self : access Other; Mode : Integer := 0) return access Other;
            overriding function Alias_Of (Self : access Other; Mode : Integer := 0) return access Other;
            overriding function Read (Self : Other) return Integer;
            Other_Item : aliased Other;
            Item : aliased Leaf := (Value => 8, Extra => 9);
        end Model;
        package body Model is
            overriding function Make (Mode : Integer := 0) return access Leaf is
            begin
                Factories := Factories + 1;
                if Mode = 1 then
                    return null;
                elsif Mode = 2 then
                    raise Constraint_Error;
                end if;
                return Item'Access;
            end Make;
            overriding function Identity (Self : access Leaf; Mode : Integer := 0) return access Leaf is
            begin
                if Mode = 1 then
                    return null;
                end if;
                return Self;
            end Identity;
            overriding function Alias_Of (Self : access Leaf; Mode : Integer := 0) return access Leaf
                renames Identity;
            overriding function Read (Self : Leaf) return Integer is
            begin
                return Self.Value + Self.Extra;
            end Read;
            overriding function Make (Mode : Integer := 0) return access Other is
            begin
                return null;
            end Make;
            overriding function Identity (Self : access Other; Mode : Integer := 0) return access Other is
            begin
                return Self;
            end Identity;
            overriding function Alias_Of (Self : access Other; Mode : Integer := 0) return access Other
                renames Identity;
            overriding function Read (Self : Other) return Integer is
            begin
                return Self.Value;
            end Read;
        end Model;
        use Access_Result_Base;
        Wide : Root'Class := Model.Item;
        Ref : access Root'Class := Model.Item'Access;
        Result : access Root'Class := Identity (Ref);
        Other_Ref : access Root'Class := Model.Other_Item'Access;
        type Factory is access function (Mode : Integer) return access Model.Leaf;
        Callback : Factory := Model.Make'Access;
    begin
        if Read (Result.all) /= 17 or else Model.Sum (Model.Item) /= 34 then
            raise Program_Error;
        end if;
        Result := Identity (Ref, 1);
        if Result /= null then
            raise Program_Error;
        end if;
        begin
            Saved := Identity (Ref);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        if Sum (Wide) /= 34 then
            raise Program_Error;
        end if;
        Wide := Make.all;
        if Read (Wide) /= 17 then
            raise Program_Error;
        end if;
        if Read (Identity (Ref).all) /= 17 or else Read (Alias_Of (Ref).all) /= 17 then
            raise Program_Error;
        end if;
        begin
            Wide := Make (1).all;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Wide := Make (2).all;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Pair (Ref, Other_Ref);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        if Factories /= 5 then
            raise Program_Error;
        end if;
        Pair (Ref, Ref);
        if Model.Read (Callback (0).all) /= 17 then
            raise Program_Error;
        end if;
        Result := Callback (1);
        if Result /= null or else Factories /= 8 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("controlling access worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("controlling access results passed");
end ControllingAccessResults;
