with Ada.Strings.Wide_Support;
-- RM A.4.2 character sets and mappings.
package Ada.Strings.Wide_Maps is
    pragma Preelaborate;
    type Wide_Character_Set is private;
    pragma Preelaborable_Initialization (Wide_Character_Set);
    Null_Set : constant Wide_Character_Set;
    type Wide_Character_Range is record
        Low : Wide_Character;
        High : Wide_Character;
    end record;
    type Wide_Character_Ranges is array (Positive range <>) of Wide_Character_Range;
    subtype Wide_Character_Sequence is Wide_String;

    function To_Set (Ranges : Wide_Character_Ranges) return Wide_Character_Set;
    function To_Set (Span : Wide_Character_Range) return Wide_Character_Set;
    function To_Set (Sequence : Wide_Character_Sequence) return Wide_Character_Set;
    function To_Set (Singleton : Wide_Character) return Wide_Character_Set;
    function To_Ranges (Set : Wide_Character_Set) return Wide_Character_Ranges;
    function To_Sequence (Set : Wide_Character_Set) return Wide_Character_Sequence;
    function "=" (Left, Right : Wide_Character_Set) return Boolean;
    function "not" (Right : Wide_Character_Set) return Wide_Character_Set;
    function "and" (Left, Right : Wide_Character_Set) return Wide_Character_Set;
    function "or" (Left, Right : Wide_Character_Set) return Wide_Character_Set;
    function "xor" (Left, Right : Wide_Character_Set) return Wide_Character_Set;
    function "-" (Left, Right : Wide_Character_Set) return Wide_Character_Set;
    function Is_In (Element : Wide_Character; Set : Wide_Character_Set) return Boolean;
    function Is_Subset (Elements : Wide_Character_Set; Set : Wide_Character_Set) return Boolean;
    function "<=" (Left, Right : Wide_Character_Set) return Boolean;

    type Wide_Character_Mapping is private;
    pragma Preelaborable_Initialization (Wide_Character_Mapping);
    Identity : constant Wide_Character_Mapping;
    function Value (Map : Wide_Character_Mapping; Element : Wide_Character) return Wide_Character;
    function To_Mapping (From, To : Wide_Character_Sequence) return Wide_Character_Mapping;
    function To_Domain (Map : Wide_Character_Mapping) return Wide_Character_Sequence;
    function To_Range (Map : Wide_Character_Mapping) return Wide_Character_Sequence;
    type Wide_Character_Mapping_Function is
        access function (From : Wide_Character) return Wide_Character;
private
    type Wide_Character_Set is record
        value : Wide_Support.Set;
    end record;
    Null_Set : constant Wide_Character_Set := (value => <>);
    type Wide_Character_Mapping is record
        value : Wide_Support.Mapping;
    end record;
    Identity : constant Wide_Character_Mapping := (value => <>);
end Ada.Strings.Wide_Maps;
