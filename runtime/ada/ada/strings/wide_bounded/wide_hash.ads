with Ada.Containers;
generic
    with package Wide_Bounded is new Ada.Strings.Wide_Bounded.Generic_Bounded_Length (<>);
function Ada.Strings.Wide_Bounded.Wide_Hash (Key : Wide_Bounded.Bounded_Wide_String) return Containers.Hash_Type;
pragma Preelaborate (Wide_Hash);
