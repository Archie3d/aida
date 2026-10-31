with Ada.Streams.Stream_IO;
with Ada.Text_IO; use Ada.Text_IO;
procedure StreamSizes is
    type Small is range -100 .. 100;
    for Small'Stream_Size use 16;
    type Byte is mod 256;
    for Byte'Stream_Size use 8;
    type Big is range -100 .. 100;
    for Big'Stream_Size use 64;
    type Items is array (1 .. 2) of Small;
    type Padded is record
        A : Byte;
        B : Big;
    end record;
    for Padded'Size use 256;
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    A : Items := (-10, 20);
    B : Padded := (255, -99);
begin
    if Small'Stream_Size /= 16 or Byte'Stream_Size /= 8 or Big'Stream_Size /= 64 then
        raise Program_Error;
    end if;
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "streamsizes.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Items'Write (Channel, A);
    Padded'Write (Channel, B);
    if Integer (Ada.Streams.Stream_IO.Size (File)) /= 13 then raise Program_Error; end if;
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "streamsizes.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    A := (others => 0);
    B := (0, 0);
    Items'Read (Channel, A);
    Padded'Read (Channel, B);
    if A /= Items'(-10, 20) or B.A /= 255 or B.B /= -99 then raise Program_Error; end if;
    Ada.Streams.Stream_IO.Delete (File);
    Put_Line ("streamsizes: passed");
end StreamSizes;
