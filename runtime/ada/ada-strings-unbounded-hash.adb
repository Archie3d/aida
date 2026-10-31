with Ada.Strings.Hash;
function Ada.Strings.Unbounded.Hash (Key : Unbounded_String) return Containers.Hash_Type is
begin
    return Ada.Strings.Hash (To_String (Key));
end Ada.Strings.Unbounded.Hash;
