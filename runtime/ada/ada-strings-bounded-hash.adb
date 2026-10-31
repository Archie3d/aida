with Ada.Strings.Hash;
function Ada.Strings.Bounded.Hash (Key : Bounded.Bounded_String) return Containers.Hash_Type is
begin
    return Ada.Strings.Hash (Bounded.To_String (Key));
end Ada.Strings.Bounded.Hash;
