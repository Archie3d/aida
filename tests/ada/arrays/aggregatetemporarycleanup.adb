with Ada.Finalization;
procedure AggregateTemporaryCleanup is
    type Token is new Ada.Finalization.Limited_Controlled with record
        Value : Integer := 0;
    end record;
    overriding procedure Finalize (Object : in out Token);
    function Make return Token;
    Calls : Integer := 0;
    Finalizations : Integer := 0;
    Fail : Boolean := True;

    procedure Finalize (Object : in out Token) is
    begin
        Finalizations := Finalizations + 1;
        if Fail and Object.Value = 104 then
            raise Constraint_Error;
        end if;
    end Finalize;

    function Make return Token is
    begin
        Calls := Calls + 1;
        return Result : Token do
            Result.Value := 100 + Calls;
        end return;
    end Make;

    type Values is array (1 .. 4) of Integer;
    Items : Values := (11, 12, 13, 14);
begin
    begin
        Items := (others => Make.Value);
        raise Constraint_Error;
    exception
        when Program_Error =>
            if Calls /= 4 or Finalizations /= 4 or Items /= (11, 12, 13, 14) then
                raise Constraint_Error;
            end if;
    end;
    Fail := False;
    Calls := 0;
    Items := (others => Make.Value);
    if Calls /= 4 or Finalizations /= 8
        or Items (1) + Items (2) + Items (3) + Items (4) /= 410 then
        raise Program_Error;
    end if;
end AggregateTemporaryCleanup;
