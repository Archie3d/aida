with Ada.Finalization;
package Elaborationearly is
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Guard);
    function Later return Integer;
    type Callback is access function return Integer;
    function Probe return Integer;
end Elaborationearly;
