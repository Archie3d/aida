package wideStringMappingSupport is
    calls : Natural := 0;
    fail : Boolean := False;
    function lower (From : Wide_Character) return Wide_Character;
    function Task_Lower (From : Wide_Character) return Wide_Character;
end wideStringMappingSupport;
