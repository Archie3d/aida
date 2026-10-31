with Ada.Strings.Maps;
procedure StringMappingErrors is
    function Nested (From : Character) return Character is
    begin
        return From;
    end Nested;
    mapping : Ada.Strings.Maps.Character_Mapping_Function := Nested'Access;
begin
    null;
end StringMappingErrors;
