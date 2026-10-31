-- RM A.4.6 Latin-1 sets and mappings; no body elaboration is required.
package Ada.Strings.Wide_Maps.Wide_Constants is
    pragma Preelaborate;
    Control_Set : constant Wide_Character_Set;
    Graphic_Set : constant Wide_Character_Set;
    Letter_Set : constant Wide_Character_Set;
    Lower_Set : constant Wide_Character_Set;
    Upper_Set : constant Wide_Character_Set;
    Basic_Set : constant Wide_Character_Set;
    Decimal_Digit_Set : constant Wide_Character_Set;
    Hexadecimal_Digit_Set : constant Wide_Character_Set;
    Alphanumeric_Set : constant Wide_Character_Set;
    Special_Set : constant Wide_Character_Set;
    ISO_646_Set : constant Wide_Character_Set;
    Lower_Case_Map : constant Wide_Character_Mapping;
    Upper_Case_Map : constant Wide_Character_Mapping;
    Basic_Map : constant Wide_Character_Mapping;
    Character_Set : constant Wide_Character_Set;
private
    Control_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Control, Data => <>));
    Graphic_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Graphic, Data => <>));
    Letter_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Letter, Data => <>));
    Lower_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Lower, Data => <>));
    Upper_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Upper, Data => <>));
    Basic_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Basic, Data => <>));
    Decimal_Digit_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Decimal_Digit, Data => <>));
    Hexadecimal_Digit_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Hexadecimal_Digit, Data => <>));
    Alphanumeric_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Alphanumeric, Data => <>));
    Special_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.Special, Data => <>));
    ISO_646_Set : constant Wide_Character_Set :=
        (value => (Prefix => 0, Base => Wide_Support.ISO_646, Data => <>));
    Lower_Case_Map : constant Wide_Character_Mapping :=
        (value => (Base => Wide_Support.Lower_Case, Data => <>));
    Upper_Case_Map : constant Wide_Character_Mapping :=
        (value => (Base => Wide_Support.Upper_Case, Data => <>));
    Basic_Map : constant Wide_Character_Mapping :=
        (value => (Base => Wide_Support.Basic_Case, Data => <>));
    Character_Set : constant Wide_Character_Set :=
        (value => (Prefix => 256, Base => Wide_Support.Empty, Data => <>));
end Ada.Strings.Wide_Maps.Wide_Constants;
