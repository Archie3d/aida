with Ada.Strings.Wide_Support;
-- RM A.4.2 character sets and mappings.
package Ada.Strings.Wide_Wide_Maps is
    pragma Preelaborate;
    type Wide_Wide_Character_Set is private;
    pragma Preelaborable_Initialization (Wide_Wide_Character_Set);
    Null_Set : constant Wide_Wide_Character_Set;
    type Wide_Wide_Character_Range is record
        Low : Wide_Wide_Character;
        High : Wide_Wide_Character;
    end record;
    type Wide_Wide_Character_Ranges is array (Positive range <>) of Wide_Wide_Character_Range;
    subtype Wide_Wide_Character_Sequence is Wide_Wide_String;

    function To_Set (Ranges : Wide_Wide_Character_Ranges) return Wide_Wide_Character_Set;
    function To_Set (Span : Wide_Wide_Character_Range) return Wide_Wide_Character_Set;
    function To_Set (Sequence : Wide_Wide_Character_Sequence) return Wide_Wide_Character_Set;
    function To_Set (Singleton : Wide_Wide_Character) return Wide_Wide_Character_Set;
    function To_Ranges (Set : Wide_Wide_Character_Set) return Wide_Wide_Character_Ranges;
    function To_Sequence (Set : Wide_Wide_Character_Set) return Wide_Wide_Character_Sequence;
    function "=" (Left, Right : Wide_Wide_Character_Set) return Boolean;
    function "not" (Right : Wide_Wide_Character_Set) return Wide_Wide_Character_Set;
    function "and" (Left, Right : Wide_Wide_Character_Set) return Wide_Wide_Character_Set;
    function "or" (Left, Right : Wide_Wide_Character_Set) return Wide_Wide_Character_Set;
    function "xor" (Left, Right : Wide_Wide_Character_Set) return Wide_Wide_Character_Set;
    function "-" (Left, Right : Wide_Wide_Character_Set) return Wide_Wide_Character_Set;
    function Is_In (Element : Wide_Wide_Character; Set : Wide_Wide_Character_Set) return Boolean;
    function Is_Subset (Elements : Wide_Wide_Character_Set; Set : Wide_Wide_Character_Set) return Boolean;
    function "<=" (Left, Right : Wide_Wide_Character_Set) return Boolean;

    type Wide_Wide_Character_Mapping is private;
    pragma Preelaborable_Initialization (Wide_Wide_Character_Mapping);
    Identity : constant Wide_Wide_Character_Mapping;
    function Value (Map : Wide_Wide_Character_Mapping; Element : Wide_Wide_Character) return Wide_Wide_Character;
    function To_Mapping (From, To : Wide_Wide_Character_Sequence) return Wide_Wide_Character_Mapping;
    function To_Domain (Map : Wide_Wide_Character_Mapping) return Wide_Wide_Character_Sequence;
    function To_Range (Map : Wide_Wide_Character_Mapping) return Wide_Wide_Character_Sequence;
    type Wide_Wide_Character_Mapping_Function is
        access function (From : Wide_Wide_Character) return Wide_Wide_Character;
private
    type Wide_Wide_Character_Set is record
        value : Wide_Support.Set;
    end record;
    Null_Set : constant Wide_Wide_Character_Set := (value => <>);
    type Wide_Wide_Character_Mapping is record
        value : Wide_Support.Mapping;
    end record;
    Identity : constant Wide_Wide_Character_Mapping := (value => <>);
end Ada.Strings.Wide_Wide_Maps;
