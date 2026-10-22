with Ada.Streams.Stream_IO;
with Ada.Text_IO; use Ada.Text_IO;
procedure EnumStreams is
    type Color is (Red, Green, Blue);
    for Color use (7, 11, 250);
    subtype Upper is Color range Green .. Blue;
    type Byte is range 0 .. 255;
    for Byte'Size use 8;
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    Item : Upper := Green;
    Count : Integer := 0;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "enumstream.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Color'Output (Channel, Blue);
    Color'Write (Channel, Red);
    Color'Write (Channel, Red);
    Byte'Write (Channel, 44);
    Byte'Write (Channel, 44);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "enumstream.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Item := Upper'Input (Channel);
    begin
        Item := Upper'Input (Channel);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        Color'Read (Channel, Item);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        Item := Upper'Input (Channel);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        Color'Read (Channel, Item);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    if Count /= 4 or Item /= Blue then raise Program_Error; end if;
    Ada.Streams.Stream_IO.Delete (File);
    Put_Line ("enumeration streams check codes and subtype copy-back");
end EnumStreams;
