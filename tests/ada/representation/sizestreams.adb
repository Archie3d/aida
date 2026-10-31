with Ada.Text_IO; use Ada.Text_IO;
with Ada.Streams.Stream_IO;
with Size_Storage; use Size_Storage;
procedure SizeStreams is
    type Open_Array is array (Integer range <>) of Byte;
    subtype Padded is Open_Array (1 .. 3);
    for Padded'Size use 64;
    type Color is (Red, Green, Blue);
    for Color use (-1000, 250, 65535);
    subtype Green_Only is Color range Green .. Green;
    for Green_Only'Size use 8;
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    X : Byte := 0;
    W : Wide := 0;
    G : Green_Only := Green;
    P : Pair := (1, 2);
    R : Roomy := (3, 4);
    A : Padded := (1, 2, 3);
    Count : Integer := 0;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "sizestream.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Long_Integer'Write (Channel, Byte'(-99));
    Long_Integer'Write (Channel, 1000);
    Wide'Write (Channel, -8);
    Color'Write (Channel, G);
    Color'Write (Channel, Blue);
    Roomy'Write (Channel, P);
    Roomy'Write (Channel, P);
    Padded'Output (Channel, A);
    Byte'Write (Channel, 88);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "sizestream.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Long_Integer'Read (Channel, X);
    if X /= -99 then raise Program_Error; end if;
    begin
        Long_Integer'Read (Channel, X);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    W := Wide'Input (Channel);
    Color'Read (Channel, G);
    begin
        Color'Read (Channel, G);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    R := Roomy'Input (Channel);
    Roomy'Read (Channel, P);
    A := Padded'Input (Channel);
    if Byte'Input (Channel) /= 88 or A (3) /= 3 or R.A /= 1 or R.B /= 2
        or P.A /= 1 or P.B /= 2 or G /= Green or W /= -8 or X /= -99 or Count /= 2 then
        raise Program_Error;
    end if;
    Ada.Streams.Stream_IO.Delete (File);
    Put_Line ("sized streams use the prefix format and checked copy-back");
end SizeStreams;
