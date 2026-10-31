package widewideStringMappingSupport is
    calls : Natural := 0;
    fail : Boolean := False;
    function lower (From : Wide_Wide_Character) return Wide_Wide_Character;
    function Task_Lower (From : Wide_Wide_Character) return Wide_Wide_Character;
end widewideStringMappingSupport;
