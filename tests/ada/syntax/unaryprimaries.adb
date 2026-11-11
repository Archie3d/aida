with Ada.Text_IO;
procedure Unaryprimaries is
    X : Integer := -7;
    Flag : Boolean := True;
begin
    if not (not Flag) /= Flag or else abs (abs X) /= 7
        or else abs (X ** 2) /= 49 or else (abs X) ** 2 /= 49 then
        raise Program_Error with "unary primary operands";
    end if;
    Ada.Text_IO.Put_Line ("unary primaries passed");
end Unaryprimaries;
