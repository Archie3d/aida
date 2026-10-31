with Ada.Strings.Wide_Hash;
function Ada.Strings.Wide_Bounded.Wide_Hash (Key : Wide_Bounded.Bounded_Wide_String) return Containers.Hash_Type is
begin
    return Ada.Strings.Wide_Hash (Wide_Bounded.To_Wide_String (Key));
end Ada.Strings.Wide_Bounded.Wide_Hash;
