with Ada.Characters.Handling;
package body widewideStringMappingSupport is
    function lower (From : Wide_Wide_Character) return Wide_Wide_Character is
    begin
        calls := calls + 1;
        if fail and From = 'B' then
            raise Program_Error;
        end if;
        return Wide_Wide_Character'Val (Character'Pos (Ada.Characters.Handling.To_Lower (Character'Val (Wide_Wide_Character'Pos (From)))));
    end lower;
    function Task_Lower (From : Wide_Wide_Character) return Wide_Wide_Character is
    begin
        if From >= 'A' and From <= 'Z' then
            return Wide_Wide_Character'Val (Wide_Wide_Character'Pos (From) + 32);
        end if;
        return From;
    end Task_Lower;
end widewideStringMappingSupport;
