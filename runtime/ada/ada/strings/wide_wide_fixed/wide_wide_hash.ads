with Ada.Containers;
with Ada.Strings.Wide_Wide_Hash;
function Ada.Strings.Wide_Wide_Fixed.Wide_Wide_Hash (Key : Wide_Wide_String) return Containers.Hash_Type
    renames Ada.Strings.Wide_Wide_Hash;
pragma Pure (Wide_Wide_Hash);
