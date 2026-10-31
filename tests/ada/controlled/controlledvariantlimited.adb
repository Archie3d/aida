with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure ControlledVariantLimited is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    type Item (Present : Boolean := True) is record
        case Present is
            when True => Value : Guard;
            when False => Number : Integer := 0;
        end case;
    end record;
    function Make (Present : Boolean) return Item is
    begin
        return Result : Item (Present) do
            null;
        end return;
    end Make;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    type Link is access Item;
    procedure Free is new Ada.Unchecked_Deallocation (Item, Link);
begin
    declare
        A : Item := Make (True);
        B : Item := Make (False);
        Heap : Link := new Item'(Make (True));
    begin
        Check (Live = 2 and A.Present and not B.Present);
        Free (Heap);
        Check (Live = 1);
        begin
            declare
                Local : Item (True);
            begin
                Check (Live = 2);
                raise Constraint_Error;
            end;
        exception
            when Constraint_Error => Check (Live = 1);
        end;
    end;
    Check (Live = 0);
    Put_Line ("limited controlled variants passed");
end ControlledVariantLimited;
