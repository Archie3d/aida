-- Shared implementation of A.4.9 and the A.4.7/8 native-width hashes.
with Ada.Containers;
generic
    type Character_Type is (<>);
    type Text_Type is array (Positive range <>) of Character_Type;
function Ada.Strings.Generic_Hash (Key : Text_Type) return Containers.Hash_Type;
pragma Pure (Generic_Hash);
