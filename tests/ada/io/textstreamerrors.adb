with Ada.Text_IO;
with Ada.Text_IO.Text_Streams;
with Ada.Wide_Text_IO;
with Ada.Wide_Text_IO.Text_Streams;
with Ada.Wide_Wide_Text_IO;
with Ada.Wide_Wide_Text_IO.Text_Streams;
procedure TextStreamErrors is
    Narrow : Ada.Text_IO.File_Type;
    Wide : Ada.Wide_Text_IO.File_Type;
    Widest : Ada.Wide_Wide_Text_IO.File_Type;
    N : Ada.Text_IO.Text_Streams.Stream_Access;
    W : Ada.Wide_Text_IO.Text_Streams.Stream_Access;
    WW : Ada.Wide_Wide_Text_IO.Text_Streams.Stream_Access;
begin
    N := Ada.Text_IO.Text_Streams.Stream (Wide);
    W := Ada.Wide_Text_IO.Text_Streams.Stream (Widest);
    WW := Ada.Wide_Wide_Text_IO.Text_Streams.Stream (Narrow);
    N := W;
    W := WW;
end TextStreamErrors;
