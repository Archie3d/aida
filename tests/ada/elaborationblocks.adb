with Elaboration_Blocks;
with Ada.Text_IO; use Ada.Text_IO;
procedure ElaborationBlocks is
begin
    if not Elaboration_Blocks.Finished then
        raise Program_Error;
    end if;
    Put_Line ("elaboration blocks passed");
end ElaborationBlocks;
