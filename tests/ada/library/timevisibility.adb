with Ada.Real_Time;
with Ada.Text_IO;
procedure TimeVisibility is
    use type Ada.Real_Time.Time;
    use type Ada.Real_Time.Time_Span;
    Rows : Natural := 24;
    Width : Natural := Rows * 3 / 4;
    Three : constant := 3;
    Named_Width : Natural := Rows * Three / 4;
    Span : Ada.Real_Time.Time_Span := Ada.Real_Time.Milliseconds (12);
    Scaled : Ada.Real_Time.Time_Span := Rows * Span;
    Ratio : Integer := Scaled / Ada.Real_Time.Milliseconds (4);
    Halved : Ada.Real_Time.Time_Span := Span / 2;
    Now : Ada.Real_Time.Time := Ada.Real_Time.Clock;
begin
    if Width /= 18 or else Named_Width /= 18 or else Ratio /= 72
        or else Halved /= Ada.Real_Time.Milliseconds (6)
        or else Now + Span - Now /= Span then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("time visibility passed");
end TimeVisibility;
