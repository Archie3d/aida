with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Controlled_Result_Model; use Controlled_Result_Model;
procedure RecordControlled is
    type Item (Present : Boolean := True) is record
        Common : Guard;
        case Present is
            when True => Value : Guard;
            when False => Number : Integer := 91;
        end case;
    end record;
    for Item use record
        Common at 32 range 0 .. 127;
        Present at 48 range 0 .. 7;
        Value at 0 range 0 .. 127;
        Number at 0 range 0 .. 31;
    end record;
    type Link is access Item;
    procedure Free is new Ada.Unchecked_Deallocation (Item, Link);
    function Make return Item is
        Local : Item;
    begin
        Local.Value.Data.Value := 42;
        return Local;
    end Make;
begin
    declare
        A : Item;
        B : Item := (False, Common => <>, Number => 7);
        Heap : Link := new Item'(Make);
    begin
        Check (Objects = 5 and Resources = 5);
        A := B;
        Check (Objects = 4 and Resources = 3 and A.Number = 7);
        B := Heap.all;
        Check (Objects = 5 and Resources = 3 and B.Value.Data.Value = 42);
        Free (Heap);
        Check (Objects = 3 and Resources = 3);
        Fail_Adjust := 2;
        begin
            A := B;
            raise Constraint_Error;
        exception
            when Program_Error => Check (Objects = 3 and Resources = 3);
        end;
        Fail_Adjust := 0;
        A := B;
        Check (Objects = 4 and Resources = 2);
    end;
    Check (Objects = 0 and Resources = 0);
    Put_Line ("controlled record representation passed");
end RecordControlled;
