with Ada.Containers;
with WideAlternativeHash;
function Ada.Strings.Wide_Fixed.Wide_Hash (Key : Wide_String) return Ada.Containers.Hash_Type
    renames WideAlternativeHash;
pragma Pure (Wide_Hash);
