with Ada.Strings.Wide_Wide_Maps;
procedure widewideStringMappingErrors is
    function Nested (From : Wide_Wide_Character) return Wide_Wide_Character is
    begin
        return From;
    end Nested;
    mapping : Ada.Strings.Wide_Wide_Maps.Wide_Wide_Character_Mapping_Function := Nested'Access;
begin
    null;
end widewideStringMappingErrors;
