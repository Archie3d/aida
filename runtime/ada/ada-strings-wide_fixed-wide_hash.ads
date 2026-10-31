with Ada.Containers;
with Ada.Strings.Wide_Hash;
function Ada.Strings.Wide_Fixed.Wide_Hash (Key : Wide_String) return Containers.Hash_Type
    renames Ada.Strings.Wide_Hash;
pragma Pure (Wide_Hash);
