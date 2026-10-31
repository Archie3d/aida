with Ada.Text_IO; use Ada.Text_IO;

procedure IntegerLiteralBounds is
    Largest : Long_Integer := 9223372036854775807;
    Scaled : Long_Integer := 922337203685477580E1;
    Based : Long_Integer := 16#7FFF_FFFF_FFFF_FFFF#;
    Based_Scaled : Long_Integer := 2#1#E62;
    Zero : Long_Integer := 0E9223372036854775807;
    Based_Zero : Long_Integer := 2#0#E9223372036854775807;
begin
    Put_Line (Long_Integer'Image (Largest));
    Put_Line (Long_Integer'Image (Scaled));
    Put_Line (Long_Integer'Image (Based));
    Put_Line (Long_Integer'Image (Based_Scaled));
    Put_Line (Long_Integer'Image (Zero + Based_Zero));
end IntegerLiteralBounds;
