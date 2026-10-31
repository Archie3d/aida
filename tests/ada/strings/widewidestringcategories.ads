with Ada.Strings.Wide_Wide_Maps.Wide_Wide_Constants;
with Ada.Strings.Wide_Wide_Bounded;
with Ada.Strings.Wide_Wide_Unbounded;
package widewideStringCategories is
    pragma Preelaborate;
    package B is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (8);
    type Bundle is private;
    pragma Preelaborable_Initialization (Bundle);
    Empty : constant Bundle;
private
    type Bundle is record
        Set : Ada.Strings.Wide_Wide_Maps.Wide_Wide_Character_Set;
        Mapping : Ada.Strings.Wide_Wide_Maps.Wide_Wide_Character_Mapping;
        Bounded : B.Bounded_Wide_Wide_String;
        Unbounded : Ada.Strings.Wide_Wide_Unbounded.Unbounded_Wide_Wide_String;
    end record;
    Empty : constant Bundle := (others => <>);
end widewideStringCategories;
