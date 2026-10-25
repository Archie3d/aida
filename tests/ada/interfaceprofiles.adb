with Ada.Text_IO;
procedure InterfaceProfiles is
    procedure Run (Seed : Integer) is
        package Contracts is
            type API is interface;
            function Read (Self : API) return Integer is abstract;
            function Identity (Self : access API) return access API is abstract;
            function "=" (Left, Right : API) return Boolean is abstract;
            function Merge (Left, Right : API) return Integer is abstract;
            function Make (Value : Integer) return access API is abstract;
        end Contracts;
        package Model is
            type Base is tagged record
                Data : Integer := Seed;
            end record;
            type Item is new Base and Contracts.API with null record;
            function Helper (Self : Item) return Integer;
            overriding function Read (Self : Item) return Integer renames Helper;
            overriding function Identity (Self : access Item) return access Item;
            overriding function "=" (Left, Right : Item) return Boolean;
            overriding function Merge (Left, Right : Item) return Integer;
            overriding function Make (Value : Integer) return access Item;
        end Model;
        package body Model is
            overriding function Make (Value : Integer) return access Item is
            begin
                return new Item'(Data => Value);
            end Make;
            function Helper (Self : Item) return Integer is
            begin
                return Self.Data;
            end Helper;
            overriding function Identity (Self : access Item) return access Item is
            begin
                return Self;
            end Identity;
            overriding function "=" (Left, Right : Item) return Boolean is
            begin
                return Left.Data = Right.Data;
            end "=";
            overriding function Merge (Left, Right : Item) return Integer is
            begin
                return Left.Data + Right.Data;
            end Merge;
        end Model;
        use type Contracts.API;
        Object : aliased Model.Item;
        Other : Model.Item;
        R : Contracts.API'Class := Object;
        S : Contracts.API'Class := Other;
        type Reader is access all Contracts.API'Class;
        Pointer : Reader := Object'Access;
        Returned : access Contracts.API'Class := Contracts.Identity (Pointer);
    begin
        if Contracts.Read (Returned.all) /= Seed or else not (R = S)
            or else Contracts.Merge (R, S) /= Seed * 2 then
            raise Program_Error;
        end if;
        Model.Item (Returned.all).Data := Seed + 1;
        R := Contracts.API'Class (Object);
        if Object.Data /= Seed + 1 or else R = S then
            raise Program_Error;
        end if;
        Pointer.all := Contracts.Make (Seed + 3).all;
        if Object.Data /= Seed + 3 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (17);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("interface profile worker failed");
    end Worker;
begin
    Run (11);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("interface profiles passed");
end InterfaceProfiles;
