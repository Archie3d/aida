with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure ControlledErrors is
    type Guard is new Ada.Finalization.Controlled with null record;
    type Limited_Guard is new Ada.Finalization.Limited_Controlled with null record;
    A : Guard;
    B : Guard := A;
    C : Guard'Class := A;
    Abstract_Object : Ada.Finalization.Controlled;
    type Guards is array (1 .. 2) of Guard;
    type Holder is record
        Item : Guard;
    end record;
    type Link is access Guard;
    P : Link := new Guard;
    procedure Free is new Ada.Unchecked_Deallocation (Guard, Link);
    function Make return Guard;
    package Hidden is
        type Private_Guard is tagged private;
    private
        type Private_Guard is new Guard with null record;
    end Hidden;
    package Abstract_View is
        type Root is abstract tagged private;
    private
        type Root is tagged null record;
    end Abstract_View;
    L, M : Limited_Guard;
begin
    A := A;
    L := M;
    if L = M then
        null;
    end if;
end ControlledErrors;
