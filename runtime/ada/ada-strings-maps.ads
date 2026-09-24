-- RM A.4.2, excluding access-to-subprogram Character_Mapping_Function.
package Ada.Strings.Maps is
    type Character_Set is private;
    Null_Set : constant Character_Set;
    type Character_Range is record
        Low : Character;
        High : Character;
    end record;
    type Character_Ranges is array (Positive range <>) of Character_Range;
    subtype Character_Sequence is String;

    function To_Set (Ranges : Character_Ranges) return Character_Set;
    function To_Set (Span : Character_Range) return Character_Set;
    function To_Set (Sequence : Character_Sequence) return Character_Set;
    function To_Set (Singleton : Character) return Character_Set;
    function To_Ranges (Set : Character_Set) return Character_Ranges;
    function To_Sequence (Set : Character_Set) return Character_Sequence;
    function "=" (Left, Right : Character_Set) return Boolean;
    function "not" (Right : Character_Set) return Character_Set;
    function "and" (Left, Right : Character_Set) return Character_Set;
    function "or" (Left, Right : Character_Set) return Character_Set;
    function "xor" (Left, Right : Character_Set) return Character_Set;
    function "-" (Left, Right : Character_Set) return Character_Set;
    function Is_In (Element : Character; Set : Character_Set) return Boolean;
    function Is_Subset (Elements : Character_Set; Set : Character_Set) return Boolean;
    function "<=" (Left, Right : Character_Set) return Boolean;

    type Character_Mapping is private;
    Identity : constant Character_Mapping;
    function Value (Map : Character_Mapping; Element : Character) return Character;
    function To_Mapping (From, To : Character_Sequence) return Character_Mapping;
    function To_Domain (Map : Character_Mapping) return Character_Sequence;
    function To_Range (Map : Character_Mapping) return Character_Sequence;
private
    type SetBits is array (Character) of Boolean;
    type Character_Set is record
        bits : SetBits := (others => False);
    end record;
    Null_Set : constant Character_Set := (bits => (others => False));

    -- Zero offsets make default-initialized mappings the identity without
    -- invoking a body during specification elaboration.
    type MappingOffsets is array (Character) of Integer;
    type Character_Mapping is record
        offsets : MappingOffsets := (others => 0);
    end record;
    Identity : constant Character_Mapping := (offsets => (others => 0));
end Ada.Strings.Maps;
