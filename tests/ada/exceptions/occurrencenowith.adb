with Ada.Text_IO; use Ada.Text_IO;
procedure Occurrencenowith is
begin
    raise Constraint_Error;
exception
    when E : others =>
        if E'Address = E'Address then
            Put_Line ("occurrence without with clause ok");
        end if;
end Occurrencenowith;
