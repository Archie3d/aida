with Ada.Exceptions; use Ada.Exceptions;
procedure Occurrencebindingerrors is
    procedure Modify (Item : in out Exception_Occurrence) is
    begin
        null;
    end Modify;
begin
    begin
        raise Constraint_Error;
    exception
        when E : Constraint_Error =>
            E := E;
            Modify (E);
            if E = E then
                null;
            end if;
        when F : others =>
            Modify (E);
    end;
    Modify (E);
end Occurrencebindingerrors;
