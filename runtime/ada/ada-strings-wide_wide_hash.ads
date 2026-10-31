with Ada.Strings.Generic_Hash;
function Ada.Strings.Wide_Wide_Hash is new Ada.Strings.Generic_Hash
    (Wide_Wide_Character, Wide_Wide_String);
pragma Pure (Wide_Wide_Hash);
