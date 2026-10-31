with Ada.Strings.Wide_Maps;
procedure wideStringMappingErrors is
    function Nested (From : Wide_Character) return Wide_Character is
    begin
        return From;
    end Nested;
    mapping : Ada.Strings.Wide_Maps.Wide_Character_Mapping_Function := Nested'Access;
begin
    null;
end wideStringMappingErrors;
