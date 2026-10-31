with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure GeneralAccess is
    type Int_Access is access all Integer;
    type Int_Constant is access constant Integer;
    type Pool_Access is access Integer;
    A : aliased Integer := 10;
    C : aliased constant Integer := 20;
    Alias : Integer renames A;
    P : Int_Access := Alias'Access;
    Read_Only : Int_Constant := C'Access;
    Pool : Pool_Access := new Integer'(30);
    From_Pool : Int_Access := Int_Access (Pool);
    procedure Free is new Ada.Unchecked_Deallocation (Integer, Int_Access);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    type Record_Type is record
        Value : aliased Integer := 40;
        Link : Int_Access;
    end record;
    R : Record_Type;
    Component : Int_Access := R.Value'Access;
    type Items is array (1 .. 3) of aliased Integer;
    Values : Items := (1, 2, 3);
    Element : Int_Access := Values (2)'Access;
    type Record_Access is access all Record_Type;
    Object : aliased Record_Type;
    Object_Link : Record_Access := Object'Access;
    type Record_Constant is access constant Record_Type;
    Object_View : Record_Constant := Record_Constant (Object_Link);
    package Tagged_Types is
        type Root is tagged record
            Value : Integer := 1;
        end record;
        type Child is new Root with record
            Extra : Integer := 2;
        end record;
    end Tagged_Types;
    use Tagged_Types;
    type Root_Link is access all Root'Class;
    type Child_Link is access all Child;
    Tagged_Object : aliased Child;
    Plain_Object : aliased Root;
    Wide : Root_Link := Tagged_Object'Access;
    Narrow : Child_Link := Child_Link (Wide);
    procedure Tagged_Formal (Item : in out Root) is
        type Local_Link is access all Root;
        View : Local_Link := Item'Access;
    begin
        View.Value := 22;
    end Tagged_Formal;
    function Outer return Int_Access is
    begin
        return A'Access;
    end Outer;
    procedure Via_Access (Target : Int_Access) is
    begin
        Target.all := Target.all + 1;
    end Via_Access;
begin
    Check (Narrow.Extra = 2);
    Narrow.Value := 21;
    Check (Tagged_Object.Value = 21);
    Tagged_Formal (Plain_Object);
    Check (Plain_Object.Value = 22);
    Wide := Plain_Object'Access;
    begin
        Narrow := Child_Link (Wide);
        raise Program_Error;
    exception
        when Constraint_Error => Check (Narrow.Extra = 2);
    end;
    Wide := null;
    Narrow := Child_Link (Wide);
    Check (Narrow = null);
    P.all := 11;
    Check (A = 11 and Read_Only.all = 20 and From_Pool.all = 30);
    Via_Access (A'Access);
    Check (A = 12 and Outer = P);
    Read_Only := Int_Constant (P);
    Check (Read_Only.all = 12);
    Component.all := 41;
    Element.all := 9;
    Check (R.Value = 41 and Values (2) = 9);
    Object_Link.Link := A'Access;
    Object_View.Link.all := 13;
    Check (A = 13 and Object_View.Value = 40);
    P := Int_Access (null);
    Check (P = null);
    P := new Integer'(99);
    Free (P);
    Check (P = null);
    Free (From_Pool);
    Pool := null;
    declare
        type Derived is new Int_Access;
        D : Derived := A'Access;
        Same : Int_Access := Int_Access (D);
    begin
        Same.all := 14;
        Check (A = 14);
    end;
    Put_Line ("general object access passed");
end GeneralAccess;
