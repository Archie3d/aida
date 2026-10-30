-- Character/String operations (RM A.3.2) and J.14 compatibility views.
with Ada.Characters.Conversions;
package Ada.Characters.Handling is
    pragma Pure;
    function Is_Control (Item : Character) return Boolean;
    function Is_Graphic (Item : Character) return Boolean;
    function Is_Letter (Item : Character) return Boolean;
    function Is_Lower (Item : Character) return Boolean;
    function Is_Upper (Item : Character) return Boolean;
    function Is_Basic (Item : Character) return Boolean;
    function Is_Digit (Item : Character) return Boolean;
    function Is_Decimal_Digit (Item : Character) return Boolean;
    function Is_Hexadecimal_Digit (Item : Character) return Boolean;
    function Is_Alphanumeric (Item : Character) return Boolean;
    function Is_Special (Item : Character) return Boolean;
    function Is_Line_Terminator (Item : Character) return Boolean;
    function Is_Mark (Item : Character) return Boolean;
    function Is_Other_Format (Item : Character) return Boolean;
    function Is_Punctuation_Connector (Item : Character) return Boolean;
    function Is_Space (Item : Character) return Boolean;
    function Is_ISO_646 (Item : Character) return Boolean;
    function To_Lower (Item : Character) return Character;
    function To_Upper (Item : Character) return Character;
    function To_Basic (Item : Character) return Character;
    function To_Lower (Item : String) return String;
    function To_Upper (Item : String) return String;
    function To_Basic (Item : String) return String;
    subtype ISO_646 is Character range Character'Val (0) .. Character'Val (127);
    function Is_ISO_646 (Item : String) return Boolean;
    function To_ISO_646 (Item : Character; Substitute : ISO_646 := ' ') return ISO_646;
    function To_ISO_646 (Item : String; Substitute : ISO_646 := ' ') return String;
    function Is_Character (Item : Wide_Character) return Boolean
        renames Ada.Characters.Conversions.Is_Character;
    function Is_String (Item : Wide_String) return Boolean
        renames Ada.Characters.Conversions.Is_String;
    function To_Character (Item : Wide_Character; Substitute : Character := ' ') return Character
        renames Ada.Characters.Conversions.To_Character;
    function To_String (Item : Wide_String; Substitute : Character := ' ') return String
        renames Ada.Characters.Conversions.To_String;
    function To_Wide_Character (Item : Character) return Wide_Character
        renames Ada.Characters.Conversions.To_Wide_Character;
    function To_Wide_String (Item : String) return Wide_String
        renames Ada.Characters.Conversions.To_Wide_String;
end Ada.Characters.Handling;
