with Ada.Tags;
with Opaque_Controlled_Model;
procedure OpaqueControlledErrors is
    subtype Alias is Opaque_Controlled_Model.Hidden;
    type Link is access Alias'Class;
    A : Alias;
    T : Ada.Tags.Tag := Alias'Tag;
    U : Ada.Tags.Tag := A'Tag;
    Name : String := Alias'External_Tag;
    type Extension is new Alias with null record;
begin
    A.Data := null;
    Opaque_Controlled_Model.Adjust (A);
    if A in Alias then
        null;
    end if;
end OpaqueControlledErrors;
