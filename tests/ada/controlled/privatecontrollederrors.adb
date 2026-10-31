with Ada.Finalization;
with Private_Controlled_Model; use Private_Controlled_Model;
procedure PrivateControlledErrors is
    A : Hidden;
    L, M : Limited_Hidden;
    type Extension is new Hidden with null record;
    package Invalid_Result is
        type Item is tagged limited private;
        function Make return Item;
        type Factory is access function return Item;
    private
        type Item is new Ada.Finalization.Limited_Controlled with null record;
    end Invalid_Result;
    package Invalid_Class_Result is
        type Item is tagged private;
        function Make return Item'Class;
        function Foreign return Item;
        pragma Import (C, Foreign, "foreign_item");
    private
        type Item is new Ada.Finalization.Controlled with null record;
    end Invalid_Class_Result;
    package Invalid_Limited is
        type Item is tagged private;
    private
        type Item is new Ada.Finalization.Limited_Controlled with null record;
    end Invalid_Limited;
begin
    A.Data := null;
    Adjust (A);
    L := M;
end PrivateControlledErrors;
