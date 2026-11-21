with Ada.Streams.Stream_IO;
with Ada.Text_IO;
procedure ArrayDescriptorStreams is
    type Wide_Text is array (Long_Integer range <>) of Character;
    type Unsigned_Index is mod 2 ** 64;
    type Unsigned_Text is array (Unsigned_Index range <>) of Character;
    Text : Wide_Text (5_000_000_000 .. 5_000_000_002) := "abc";
    High_Text : Unsigned_Text (Unsigned_Index'Last - 1 .. Unsigned_Index'Last) := ('d', 'e');
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "arraydescriptorstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Wide_Text'Output (Channel, Text);
    Unsigned_Text'Output (Channel, High_Text);
    Unsigned_Text'Output (Channel, High_Text);
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
    declare
        First : Unsigned_Index := Unsigned_Index'Input (Channel);
        Last : Unsigned_Index := Unsigned_Index'Input (Channel);
        Back : Unsigned_Text (First .. Last);
    begin
        Unsigned_Text'Read (Channel, Back);
        if First /= Unsigned_Index'Last - 1 or else Last /= Unsigned_Index'Last or else Back /= High_Text then
            raise Program_Error;
        end if;
    end;
    declare
        Back : Unsigned_Text := Unsigned_Text'Input (Channel);
    begin
        if Back'First /= High_Text'First or else Back'Last /= High_Text'Last or else Back /= High_Text then
            raise Program_Error;
        end if;
    end;
    Ada.Streams.Stream_IO.Close (File);
    -- String uses Integer base bounds. Write invalid non-null bounds in that
    -- format instead of reinterpreting Long_Integer bounds as Integer values.
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "arraydescriptorstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Integer'Write (Channel, -2);
    Integer'Write (Channel, -1);
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
