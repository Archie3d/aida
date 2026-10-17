with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure DerivedAccess is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 7;
    end record;
    overriding procedure Initialize (Item : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Finalize (Item : in out Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    procedure Run is
        type Link is access Guard;
        type Sibling is new Link;
        procedure Free is new Ada.Unchecked_Deallocation (Guard, Sibling);
        Kept : Link;
    begin
        declare
            subtype Alias_Link is Link;
            type Child is new Alias_Link;
            type Grandchild is new Child;
            Object : Grandchild := new Guard;
        begin
            Kept := Link (Object);
            Put_Line (Boolean'Image (Child (Kept) /= null));
        end;
        Put_Line (Integer'Image (Live));
        Put_Line (Integer'Image (Kept.Value));
        declare
            Object : Sibling := Sibling (Kept);
        begin
            Free (Object);
            Put_Line (Boolean'Image (Object = null));
        end;
        Put_Line (Integer'Image (Live));
        declare
            type Child is new Link;
            Object : Child := new Guard;
        begin
            Kept := Link (Object);
        end;
        Put_Line (Integer'Image (Live));
    end Run;
    function Identity (N : Integer) return Integer is
    begin
        return N;
    end Identity;
    type Callback is access function (N : Integer) return Integer;
    type Derived_Callback is new Callback;
    F : Callback := Identity'Access;
    G : Derived_Callback := Derived_Callback (F);
begin
    Run;
    Put_Line (Integer'Image (Live));
    Put_Line (Integer'Image (G (23)));
end DerivedAccess;
