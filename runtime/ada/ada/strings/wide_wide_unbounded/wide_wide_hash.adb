with Ada.Strings.Wide_Wide_Hash;
function Ada.Strings.Wide_Wide_Unbounded.Wide_Wide_Hash (Key : Unbounded_Wide_Wide_String) return Containers.Hash_Type is
begin
    return Ada.Strings.Wide_Wide_Hash (To_Wide_Wide_String (Key));
end Ada.Strings.Wide_Wide_Unbounded.Wide_Wide_Hash;
