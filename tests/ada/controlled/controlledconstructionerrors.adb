with Ada.Finalization;
procedure ControlledConstructionErrors is
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
    function Copy return Guard is
        Local : Guard;
    begin
        return Local;
    end Copy;
    type Guards is array (Integer range <>) of Guard;
    subtype Two is Guards (1 .. 2);
    function Wrong_Bounds return Two is
    begin
        return Result : Guards (1 .. 3);
    end Wrong_Bounds;
    function Wrong_Return return Guard is
    begin
        return Result : Guard do
            return Result;
        end return;
    end Wrong_Return;
    Local : Guard;
    Wide : Guard'Class := Local;
begin
    null;
end ControlledConstructionErrors;
