with Ada.Real_Time; use Ada.Real_Time;
procedure TimeErrors is
    type Other_Duration is delta 0.01 range -10.0 .. 10.0;
    other : Other_Duration := 0.1;
    type Other_Time is new Time;
    wrong : Other_Time;
    span : Time_Span := Time_Span_Zero;
    instant : Time := 0;
    scalar : Long_Integer := 0;
begin
    delay other;
    delay span;
    delay True;
    delay until 0;
    delay until 0.1;
    delay until other;
    delay until span;
    delay until wrong;
    scalar := Long_Integer (Clock);
    span := span * span;
end TimeErrors;
