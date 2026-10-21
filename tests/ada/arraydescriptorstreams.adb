with Ada.Streams.Stream_IO;
with Ada.Text_IO;
procedure ArrayDescriptorStreams is
    type Wide_Text is array (Long_Integer range <>) of Character;
    Text : Wide_Text (5_000_000_000 .. 5_000_000_002) := "abc";
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "arraydescriptorstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Wide_Text'Output (Channel, Text);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "arraydescriptorstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    declare
        Back : Wide_Text := Wide_Text'Input (Channel);
    begin
        if Back'First /= Text'First or Back'Last /= Text'Last or Back /= Text then
            raise Program_Error;
        end if;
    end;
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "arraydescriptorstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    begin
        declare
            Invalid_Bounds : String := String'Input (Channel);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    Ada.Streams.Stream_IO.Delete (File);
    Ada.Text_IO.Put_Line ("wide array stream bounds");
end ArrayDescriptorStreams;
