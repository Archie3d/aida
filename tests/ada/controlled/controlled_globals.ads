with Controlled_Shutdown_Model; use Controlled_Shutdown_Model;
package Controlled_Globals is
    First : Guard;
    Copy : Guard := First;
    Alias : Guard renames First;
    type Pair is record
        Left, Right : Guard;
    end record;
    Parts : Pair;
    type Guards is array (1 .. 2) of Guard;
    Items : Guards;
private
    Hidden : Guard;
end Controlled_Globals;
