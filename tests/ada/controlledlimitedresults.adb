with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Ada.Text_IO; use Ada.Text_IO;
with System;
procedure ControlledLimitedResults is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Limited_Controlled with record
        Value : Integer := 0;
        Original : System.Address;
    end record;
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
        Object.Original := Object'Address;
    end Initialize;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    function Make (Value : Integer) return Guard is
    begin
        return Result : Guard do
            Result.Value := Value;
            if Value > 0 then
                return;
            end if;
            raise Constraint_Error;
        exception
            when Constraint_Error => Result.Value := 99;
        end return;
    end Make;
    function Forward (Depth : Integer) return Guard is
    begin
        if Depth > 0 then
            return Forward (Depth - 1);
        end if;
        return Make (42);
    end Forward;
    function Retry return Guard is
    begin
        return Result : Guard do
            raise Constraint_Error;
        end return;
    exception
        when Constraint_Error => return Make (11);
    end Retry;
    type Holder is limited record
        Item : Guard;
    end record;
    function Build return Holder is
    begin
        return (Item => Make (5));
    end Build;
    function From_Holder (Object : Holder) return Guard is
    begin
        return Make (Object.Item.Value);
    end From_Holder;
    function Nested_Argument return Holder is
    begin
        return (Item => From_Holder ((Item => Make (6))));
    end Nested_Argument;
    type Guards is array (Integer range <>) of Guard;
    subtype Two is Guards (1 .. 2);
    function Many (Count : Integer) return Guards is
    begin
        return Result : Guards (3 .. Count + 2) do
            if Count > 0 then
                Result (3).Value := Count;
            end if;
        end return;
    end Many;
    function Many_Forward (Count : Integer) return Guards is
    begin
        return Many (Count);
    end Many_Forward;
    function Many_Aggregate return Guards is
    begin
        return (4 => Make (4), 5 => Make (5));
    end Many_Aggregate;
    function Wide return Guard'Class is
    begin
        return Object : Guard do
            Object.Value := 15;
        end return;
    end Wide;
    function Fail_Wide return Guard'Class is
        type Failure is new Ada.Finalization.Limited_Controlled with null record;
        overriding procedure Finalize (Object : in out Failure) is
        begin
            raise Constraint_Error;
        end Finalize;
        Fails_On_Exit : Failure;
    begin
        return Result : Guard;
    end Fail_Wide;
    function Fail_Escape return Guard'Class is
        type Local is new Guard with null record;
    begin
        return Result : Local do
            return;
        end return;
    end Fail_Escape;
    function Fixed_Many return Guards is
    begin
        return Result : Two;
    end Fixed_Many;
    function Inferred return Guards is
    begin
        return Result : Guards := Many (2) do
            Result (3).Value := 16;
        end return;
    end Inferred;
    type Factory is access function (Value : Integer) return Guard;
    Callback : Factory := Make'Access;
    type Link is access Guard;
    procedure Free is new Ada.Unchecked_Deallocation (Guard, Link);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    use type System.Address;
begin
    declare
        A : Guard := Forward (3);
        B : Guard := Callback (7);
        Heap : Link := new Guard'(Make (9));
        C : Guard := Retry;
        D : Holder := Build;
        E : Holder := Nested_Argument;
    begin
        Check (Live = 6 and A.Value = 42 and B.Value = 7 and Heap.Value = 9);
        Check (A.Original = A'Address and B.Original = B'Address and Heap.Original = Heap.all'Address);
        Free (Heap);
        Check (Live = 5 and C.Value = 11 and D.Item.Value = 5 and E.Item.Value = 6);
        Check (C.Original = C'Address and D.Item.Original = D.Item'Address);
    end;
    Check (Live = 0);
    declare
        A : Guards := Many_Forward (3);
        B : Two := Many (2);
        C : Guards := Many_Aggregate;
    begin
        Check (Live = 7 and A'First = 3 and A'Length = 3 and B (1).Value = 2 and C (4).Value = 4);
        Check (A (3).Original = A (3)'Address and B (1).Original = B (1)'Address and C (5).Original = C (5)'Address);
        begin
            declare
                Wrong : Two := Many (3);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => Check (Live = 7);
        end;
    end;
    Check (Live = 0);
    declare
        A : Guard'Class := Wide;
        B : Guards := Inferred;
        type Wide_Link is access Guard'Class;
        procedure Free is new Ada.Unchecked_Deallocation (Guard'Class, Wide_Link);
        Heap : Wide_Link := new Guard'Class'(Wide);
    begin
        Check (Live = 4 and A.Value = 15 and B (3).Value = 16 and Heap.Value = 15);
        Check (A.Original = A'Address and B (3).Original = B (3)'Address and Heap.Original = Heap.all'Address);
        Free (Heap);
        begin
            Heap := new Guard'Class'(Fail_Wide);
            raise Constraint_Error;
        exception
            when Program_Error => Check (Live = 3 and Heap = null);
        end;
        begin
            Heap := new Guard'Class'(Fail_Escape);
            raise Constraint_Error;
        exception
            when Program_Error => Check (Live = 3 and Heap = null);
        end;
        declare
            Fixed_Result : Guards := Fixed_Many;
        begin
            Check (Live = 5 and Fixed_Result'First = 1 and Fixed_Result'Last = 2);
            Check (Fixed_Result (1).Original = Fixed_Result (1)'Address);
        end;
    end;
    Check (Live = 0);
    Put_Line ("limited results built in place");
end ControlledLimitedResults;
