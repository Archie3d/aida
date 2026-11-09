with Ada.Finalization;
package body Prefixed_Layout_Library is
    Count : Integer := 0;
    function Size return Integer is begin return 3; end Size;
    N : Integer := Size;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 130;
    end record;
    overriding procedure Finalize (Object : in out Guard);
    function Get (Object : Guard) return Integer;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Count := Count + 1;
    end Finalize;
    function Get (Object : Guard) return Integer is begin return Object.Value; end Get;
    type Guards is array (1 .. N) of Guard;
    function Read return Integer renames
        Guards'(others => (Ada.Finalization.Controlled with Value => 130))(2).Get;
    function Finalized return Integer is begin return Count; end Finalized;
end Prefixed_Layout_Library;
