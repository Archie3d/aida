with Ada.Finalization;
package body Prefixed_View_Library is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 73;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    function Make return Guard;
    function Get (Object : Guard) return Integer;
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Adjust (Object : in out Guard) is
    begin
        Live := Live + 1;
    end Adjust;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Live := Live - 1;
    end Finalize;
    function Make return Guard is
        Object : Guard;
    begin
        return Object;
    end Make;
    function Get (Object : Guard) return Integer is
    begin
        return Object.Value;
    end Get;
    function Read return Integer renames Make.Get;
    function LiveCount return Integer is
    begin
        return Live;
    end LiveCount;
end Prefixed_View_Library;
