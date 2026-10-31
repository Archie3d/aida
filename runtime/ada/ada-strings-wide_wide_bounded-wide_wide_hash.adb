with Ada.Strings.Wide_Wide_Hash;
function Ada.Strings.Wide_Wide_Bounded.Wide_Wide_Hash (Key : Wide_Wide_Bounded.Bounded_Wide_Wide_String) return Containers.Hash_Type is
begin
    return Ada.Strings.Wide_Wide_Hash (Wide_Wide_Bounded.To_Wide_Wide_String (Key));
end Ada.Strings.Wide_Wide_Bounded.Wide_Wide_Hash;
