with Ada.Direct_IO;
with Ada.Streams;
with Ada.Streams.Stream_IO;
procedure PredefinedProfileErrors is
    package Files renames Ada.Streams.Stream_IO;
    package Direct is new Ada.Direct_IO (Integer);
    package Other is new Ada.Direct_IO (Integer);
    Binary : Files.File_Type;
    File : Direct.File_Type;
    Number : Integer := 1;
    Count : Files.Count := Number;
    Wrong : Other.Count := Direct.Count'(1);
    Index : Integer := Files.Index (Binary);
    Byte : Ada.Streams.Stream_Element := Number;
begin
    Files.Set_Index (Binary, Number);
    Direct.Set_Index (File, Files.Positive_Count'(1));
end PredefinedProfileErrors;
