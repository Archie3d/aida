with Ada.Real_Time;
procedure PrivateNumericLiteralErrors is
    Span : Ada.Real_Time.Time_Span := 3;
    Three : constant := 3;
    Named_Span : Ada.Real_Time.Time_Span := Three;
    package Numbers is
        type Real_Number is private;
    private
        type Real_Number is digits 6;
    end Numbers;
    Real_Value : Numbers.Real_Number := 1.0;
begin
    null;
end PrivateNumericLiteralErrors;
