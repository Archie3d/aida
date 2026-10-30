procedure Wideambiguityerrors is
    procedure Accept_Value (Item : Wide_Character) is begin null; end Accept_Value;
    procedure Accept_Value (Item : Wide_Wide_Character) is begin null; end Accept_Value;
    procedure Accept_Text (Item : Wide_String) is begin null; end Accept_Text;
    procedure Accept_Text (Item : Wide_Wide_String) is begin null; end Accept_Text;
begin
    Accept_Value ('Ω');
    Accept_Text ("Ω");
end Wideambiguityerrors;
