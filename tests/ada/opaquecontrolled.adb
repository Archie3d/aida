with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Controlled_Result_Model; use Controlled_Result_Model;
with Opaque_Controlled_Model;
with Ada.Text_IO; use Ada.Text_IO;
procedure OpaqueControlled is
    procedure Free is new Ada.Unchecked_Deallocation (Opaque_Controlled_Model.Hidden, Opaque_Controlled_Model.Link);
    type Holder is record
        Item : Opaque_Controlled_Model.Alias;
    end record;
    type Items is array (1 .. 2) of Opaque_Controlled_Model.Alias;
    procedure Local (Depth : Integer) is
        Live : Integer := 0;
        package Nested is
            type Guard is private;
            subtype Alias is Guard;
        private
            type Guard is new Ada.Finalization.Controlled with null record;
            overriding procedure Initialize (Object : in out Guard);
            overriding procedure Adjust (Object : in out Guard);
            overriding procedure Finalize (Object : in out Guard);
        end Nested;
        package body Nested is
            procedure Initialize (Object : in out Guard) is
            begin
                Live := Live + 1;
            end Initialize;
            procedure Adjust (Object : in out Guard) is
            begin
                Live := Live + 1;
            end Adjust;
            procedure Finalize (Object : in out Guard) is
            begin
                Live := Live - 1;
            end Finalize;
        end Nested;
    begin
        declare
            A : Nested.Alias;
            B : Nested.Guard := A;
        begin
            Check (Live = 2);
            if Depth > 0 then
                Local (Depth - 1);
            end if;
            A := B;
            Check (Live = 2);
        end;
        Check (Live = 0);
    end Local;
begin
    declare
        A : Opaque_Controlled_Model.Alias := Opaque_Controlled_Model.Make (42);
        B : Opaque_Controlled_Model.Hidden := A;
        C : Holder;
        D : Items;
        Heap : Opaque_Controlled_Model.Link := new Opaque_Controlled_Model.Hidden'(A);
        Limited_Object : Opaque_Controlled_Model.Limited_Hidden;
    begin
        Check (Objects = 6 and Resources = 4 and Opaque_Controlled_Model.Limited_Live = 1);
        Check (Opaque_Controlled_Model.Value (A) = 42 and Opaque_Controlled_Model.Value (B) = 42 and Opaque_Controlled_Model.Value (Heap.all) = 42);
        A := B;
        C.Item := A;
        Check (Objects = 6 and Resources = 3 and Opaque_Controlled_Model.Adjustments > 0);
        Free (Heap);
        Check (Heap = null and Objects = 5);
        Local (3);
        Fail_Adjust := 1;
        begin
            declare
                Failed : Opaque_Controlled_Model.Alias := A;
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => null;
        end;
        Check (Objects = 5 and Resources = 3);
        Fail_Adjust := 1;
        begin
            Heap := new Opaque_Controlled_Model.Hidden'(A);
            raise Constraint_Error;
        exception
            when Program_Error => null;
        end;
        Check (Heap = null and Objects = 5 and Resources = 3);
    end;
    Check (Objects = 0 and Resources = 0 and Opaque_Controlled_Model.Limited_Live = 0);
    declare
        Object : Opaque_Controlled_Model.Box_Alias := Opaque_Controlled_Model.Build;
    begin
        Check (Objects = 1 and Resources = 1);
    end;
    Check (Objects = 0 and Resources = 0);
    Put_Line ("opaque controlled types passed");
end OpaqueControlled;
