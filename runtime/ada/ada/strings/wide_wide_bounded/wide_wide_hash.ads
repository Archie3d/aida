with Ada.Containers;
generic
    with package Wide_Wide_Bounded is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (<>);
function Ada.Strings.Wide_Wide_Bounded.Wide_Wide_Hash (Key : Wide_Wide_Bounded.Bounded_Wide_Wide_String) return Containers.Hash_Type;
pragma Preelaborate (Wide_Wide_Hash);
