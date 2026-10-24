with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
procedure PrefixedCleanup is
    Live : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 5;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    function Read (Object : Guard; Fail : Boolean := False) return Integer;
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
    function Read (Object : Guard; Fail : Boolean := False) return Integer is
    begin
        if Fail then raise Constraint_Error; end if;
        return Object.Value;
    end Read;
    function Make return Guard is
        Object : Guard;
    begin
        return Object;
    end Make;
    procedure Exercise (Fail : Boolean) is
        Value : Integer := Make.Read (Fail);
    begin
        if Value /= 5 then raise Program_Error; end if;
    end Exercise;
begin
    Exercise (False);
    if Live /= 0 then raise Program_Error; end if;
    begin
        Exercise (True);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if Live /= 0 then raise Program_Error; end if;
    Put_Line ("prefixed cleanup passed");
end PrefixedCleanup;
