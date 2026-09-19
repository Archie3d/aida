with Ada.Text_IO; use Ada.Text_IO;
with Ada.Exceptions; use Ada.Exceptions;
with System;
with Occurrencepackage;

procedure Occurrencebindings is
    Original, Other : exception;
    E : Integer := 42;
    Visits : Integer := 0;

    -- Inspect the current pointer-sized identity payload without adding a
    -- public inspection operation before the next implementation phase.
    function Compare_Bytes (Left, Right : System.Address; Count : Long_Integer) return Integer;
    pragma Import (C, Compare_Bytes, "memcmp");

    procedure Observe (Item : Exception_Occurrence; Expected : System.Address) is
    begin
        if Item'Address /= Expected then
            raise Program_Error;
        end if;
        Visits := Visits + 1;
    end Observe;

    procedure Recurse (Depth : Integer) is
    begin
        raise Original;
    exception
        when E : Original =>
            declare
                Saved : System.Address := E'Address;
                procedure Capture is
                begin
                    Observe (E, Saved);
                end Capture;
            begin
                Capture;
                if Depth > 0 then
                    Recurse (Depth - 1);
                end if;
                begin
                    raise Other;
                exception
                    when Inner : others =>
                        if Inner'Address = E'Address then
                            raise Program_Error;
                        end if;
                        if Compare_Bytes (Inner'Address, E'Address, 8) = 0 then
                            raise Program_Error;
                        end if;
                        Observe (Inner, Inner'Address);
                        Capture;
                end;
                begin
                    raise Original;
                exception
                    when Again : Original =>
                        if Compare_Bytes (Again'Address, E'Address, 8) /= 0 then
                            raise Program_Error;
                        end if;
                end;
                Capture;
            end;
    end Recurse;
begin
    Recurse (2);
    begin
        begin
            raise Original;
        exception
            -- Resolve the exception choice before introducing its namesake.
            when Original : Original =>
                Observe (Original, Original'Address);
                raise;
        end;
    exception
        when E : Original | Other => Observe (E, E'Address);
    end;
    if E /= 42 or Visits /= 14 or not Occurrencepackage.Handled then
        raise Program_Error;
    end if;
    Put_Line ("occurrence bindings ok");
end Occurrencebindings;
