with Context_Worker;
with Ada.Text_IO; use Ada.Text_IO;
procedure Generatedcontexts is
    function Exercise return Integer;
    pragma Import (C, Exercise, "exerciseContexts");
begin
    if Exercise /= 0 then
        raise Program_Error with "generated context isolation failed";
    end if;
    Put_Line ("generated contexts ok");
end Generatedcontexts;
