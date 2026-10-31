with Ada.Characters.Handling;
package body StringMappingSupport is
    function lower (From : Character) return Character is
    begin
        calls := calls + 1;
        if fail and From = 'B' then
            raise Program_Error;
        end if;
        return Ada.Characters.Handling.To_Lower (From);
    end lower;
end StringMappingSupport;
