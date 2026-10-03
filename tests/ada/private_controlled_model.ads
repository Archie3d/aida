with Ada.Finalization;
with Controlled_Result_Model;
package Private_Controlled_Model is
    type Hidden is tagged private;
    subtype Alias is Hidden;
    type Link is access Hidden;
    function Make (Value : Integer) return Hidden;
    function Value (Object : Hidden) return Integer;
    overriding procedure Finalize (Object : in out Hidden);
    type Box is private;
    subtype Box_Alias is Box;
    function Build return Box;
    type Limited_Hidden is tagged limited private;
    Limited_Live : Integer := 0;
    Adjustments : Integer := 0;
private
    type Hidden is new Controlled_Result_Model.Guard with null record;
    overriding procedure Adjust (Object : in out Hidden);
    type Box is record
        Item : Hidden;
    end record;
    type Limited_Hidden is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Initialize (Object : in out Limited_Hidden);
    overriding procedure Finalize (Object : in out Limited_Hidden);
end Private_Controlled_Model;
