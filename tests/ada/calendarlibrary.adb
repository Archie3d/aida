with Ada.Calendar;
with Ada.Text_IO;
procedure CalendarLibrary is
    use Ada.Calendar;
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error;
        end if;
    end Check;
    T : Time := Time_Of (2000, 2, 29, 12345.25);
    Y : Year_Number;
    M : Month_Number;
    D : Day_Number;
    S : Day_Duration;
    Caught : Boolean := False;
begin
    Split (T, Y, M, D, S);
    Check (Y = 2000 and M = 2 and D = 29 and S = 12345.25);
    Check (Year (T) = Y and Month (T) = M and Day (T) = D and Seconds (T) = S);
    Check (Time_Of (2000, 2, 29, 86400.0) = Time_Of (2000, 3, 1));
    Check (Time_Of (1901, 1, 1) < Time_Of (2399, 12, 31));
    Check (Time_Of (2100, 3, 1) - Time_Of (2100, 2, 28) = 86400.0);
    Check ((T + 1.5) - T = 1.5 and (1.5 + T) - 1.5 = T);
    Check (T <= T and T >= T and T + 1.0 > T);
    begin
        T := Time_Of (2100, 2, 29);
    exception
        when Time_Error => Caught := True;
    end;
    Check (Caught);
    Caught := False;
    begin
        T := Time_Of (1901, 1, 1) - 1.0;
    exception
        when Time_Error => Caught := True;
    end;
    Check (Caught);
    Caught := False;
    begin
        S := Time_Of (2399, 12, 31) - Time_Of (1901, 1, 1);
    exception
        when Time_Error => Caught := True;
    end;
    Check (Caught);
    T := Clock;
    delay until T - 1.0;
    delay until Clock + 0.001;
    Ada.Text_IO.Put_Line ("calendar library passed");
end CalendarLibrary;
