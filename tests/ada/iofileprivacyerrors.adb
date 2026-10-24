with Ada.Text_IO;
with Ada.Sequential_IO;
with Ada.Direct_IO;
with Ada.Streams.Stream_IO;
procedure IOFilePrivacyErrors is
    package Sequential is new Ada.Sequential_IO (Integer);
    package Direct is new Ada.Direct_IO (Integer);
    Text_A, Text_B : Ada.Text_IO.File_Type;
    Seq_A, Seq_B : Sequential.File_Type;
    Direct_A, Direct_B : Direct.File_Type;
    Stream_A, Stream_B : Ada.Streams.Stream_IO.File_Type;
    Text_Handle : Ada.Text_IO.File_Handle;
    Seq_Handle : Sequential.File_Handle;
    Direct_Handle : Direct.File_Handle;
    Stream_Handle : Ada.Streams.Stream_IO.File_Handle;
begin
    Text_A := Text_B;
    Seq_A := Seq_B;
    Direct_A := Direct_B;
    Stream_A := Stream_B;
    if Text_A = Text_B then null; end if;
    if Seq_A = Seq_B then null; end if;
    if Direct_A = Direct_B then null; end if;
    if Stream_A = Stream_B then null; end if;
    Text_A.Handle := null;
    Seq_A.Handle := null;
    Direct_A.Handle := null;
    Stream_A.Handle := null;
end IOFilePrivacyErrors;
