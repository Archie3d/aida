with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
package body Runtime_Array_Lifetime is
    N : Integer := 2;
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 42;
    end record;
    overriding procedure Initialize (X : in out Guard);
    overriding procedure Adjust (X : in out Guard);
    overriding procedure Finalize (X : in out Guard);
    overriding procedure Initialize (X : in out Guard) is
    begin
        Live := Live + 1;
    end Initialize;
    overriding procedure Adjust (X : in out Guard) is
    begin
        Live := Live + 1;
    end Adjust;
    overriding procedure Finalize (X : in out Guard) is
    begin
        if X.Value /= 42 then raise Program_Error; end if;
        Live := Live - 1;
        if Live = 0 then Put_Line ("library arrays finalized"); end if;
    end Finalize;
    type Guards is array (Integer range <>) of Guard;
    A : Guards (1 .. N);
    B : Guards := A;
    type Limited_Guard is new Ada.Finalization.Limited_Controlled with record
        Value : Integer := 43;
    end record;
    overriding procedure Finalize (X : in out Limited_Guard) is
    begin
        if X.Value /= 43 then raise Program_Error; end if;
        Put_Line ("limited library element finalized");
    end Finalize;
    type Limited_Guards is array (Integer range <>) of Limited_Guard;
    function Make return Limited_Guards is
    begin
        return (1 .. N => (Ada.Finalization.Limited_Controlled with Value => 43));
    end Make;
    C : Limited_Guards := Make;
    procedure Check is
        Copy : Guards := B;
    begin
        if Live /= 6 or Copy (2).Value /= 42 or C (2).Value /= 43 then
            raise Program_Error;
        end if;
    end Check;
end Runtime_Array_Lifetime;
