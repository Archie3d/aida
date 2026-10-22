with Ada.Streams.Stream_IO;
with Ada.Text_IO; use Ada.Text_IO;
procedure FixedStreams is
    type Fixed is delta 0.25 range -10.0 .. 10.0;
    for Fixed'Small use 0.125;
    Upper : Fixed := 2.0;
    subtype Window is Fixed range 1.0 .. Upper;
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    Item : Window := 1.25;
    Count : Integer := 0;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "fixedstream.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Fixed'Output (Channel, 1.5);
    Fixed'Write (Channel, 3.0);
    Fixed'Write (Channel, 3.0);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "fixedstream.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Item := Window'Input (Channel);
    begin
        Item := Window'Input (Channel);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    begin
        Fixed'Read (Channel, Item);
    exception
        when Constraint_Error => Count := Count + 1;
    end;
    if Count /= 2 or Item /= 1.5 then
        raise Program_Error;
    end if;
    Ada.Streams.Stream_IO.Delete (File);
    Put_Line ("fixed streams check dynamic constraints before copy-back");
end FixedStreams;
