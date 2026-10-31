with Ada.Streams; use Ada.Streams;
with Ada.Streams.Stream_IO;
with Ada.Text_IO; use Ada.Text_IO;
procedure StreamClasswide is
    package Types is
        type Root is tagged record
            Value : Integer := 0;
        end record;
        procedure Read_Root (Stream : access Root_Stream_Type'Class; Item : out Root);
        procedure Write_Root (Stream : access Root_Stream_Type'Class; Item : Root);
        for Root'Read use Read_Root;
        for Root'Write use Write_Root;
        type Child is new Root with record
            Extra : Integer := 0;
        end record;
    end Types;
    Bias : Integer := 10;
    package body Types is
        procedure Read_Root (Stream : access Root_Stream_Type'Class; Item : out Root) is
        begin
            Item.Value := Integer'Input (Stream) - Bias;
        end Read_Root;
        procedure Write_Root (Stream : access Root_Stream_Type'Class; Item : Root) is
        begin
            Integer'Write (Stream, Item.Value + Bias);
        end Write_Root;
    end Types;
    use Types;
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    A : Child := (Root'(Value => 12) with Extra => 34);
    B : Root'Class := Child'(Root'(Value => -1) with Extra => -2);
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "streamclasswide.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Root'Class'Output (Channel, A);
    Root'Class'Write (Channel, A);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "streamclasswide.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    declare
        Back : Root'Class := Root'Class'Input (Channel);
    begin
        if Back not in Child or Back.Value /= 12 or Child (Back).Extra /= 34 then
            raise Program_Error;
        end if;
    end;
    Root'Class'Read (Channel, B);
    if B.Value /= 12 or Child (B).Extra /= 34 then raise Program_Error; end if;
    Ada.Streams.Stream_IO.Delete (File);
    Put_Line ("streamclasswide: passed");
end StreamClasswide;
