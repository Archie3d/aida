with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure ControlledComponentErrors is
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
    type Holder is record
        Item : Guard;
    end record;
    A : Guard;
    B : Holder := (Item => A);
    type Guards is array (1 .. 2) of Guard;
    Items : Guards := (others => A);
    type Default_Copy is record
        Item : Guard := A;
    end record;
    X, Y : Holder;
    Z : Holder := X;
    type Link is access Holder;
    P : Link := new Holder;
    procedure Free is new Ada.Unchecked_Deallocation (Holder, Link);
    function Make return Holder;
    type Callback is access function return Holder;
    type Other_Guards is array (1 .. 2) of Guard;
    Other : Other_Guards := Other_Guards (Items);
    type Variant (Which : Boolean) is record
        case Which is
            when True => Item : Guard;
            when False => null;
        end case;
    end record;
begin
    X := Y;
end ControlledComponentErrors;
