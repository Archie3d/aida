with Ada.Strings.Generic_Hash;
function Ada.Strings.Hash is new Ada.Strings.Generic_Hash
    (Character, String);
pragma Pure (Hash);
