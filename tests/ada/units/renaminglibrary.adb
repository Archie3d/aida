with Ada.Text_IO; use Ada.Text_IO;
with Renaming_View;
with Renaming_Chain;
with Renaming_Generator_View;
procedure RenamingLibrary is
    package Instance is new Renaming_Generator_View;
    function Calculate is new Renaming_Chain;
    function Other is new Renaming_Chain (Initial => 30);
begin
    Renaming_View.Increase;
    if Renaming_View.Value /= 12 or Calculate /= 22 or Other (4) /= 34 or Instance.Value /= 17 then
        raise Program_Error;
    end if;
    Put_Line ("library renaming passed");
end RenamingLibrary;
