with Ada.Strings.Generic_Hash;
function Ada.Strings.Wide_Hash is new Ada.Strings.Generic_Hash
    (Wide_Character, Wide_String);
pragma Pure (Wide_Hash);
