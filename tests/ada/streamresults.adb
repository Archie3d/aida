with Ada.Streams.Stream_IO;
with Ada.Text_IO; use Ada.Text_IO;
procedure Streamresults is
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    procedure Validate (A, B, C, D, E, F : String) is
    begin
        if A /= "one" or B /= "two" or C /= "three" or D /= "four" or E /= "five" or F /= "six" then
            raise Program_Error with "stream result overwritten";
        end if;
    end Validate;
    Large : String (5 .. 9004) := (others => 'x');
    Empty : String (8 .. 3);
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "streamresults.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    String'Output (Channel, "one");
    String'Output (Channel, "two");
    String'Output (Channel, "three");
    String'Output (Channel, "four");
    String'Output (Channel, "five");
    String'Output (Channel, "six");
    String'Output (Channel, Large);
    String'Output (Channel, Empty);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "streamresults.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Validate (String'Input (Channel), String'Input (Channel), String'Input (Channel),
              String'Input (Channel), String'Input (Channel), String'Input (Channel));
    declare
        Back : String := String'Input (Channel);
        Null_Back : String := String'Input (Channel);
    begin
        if Back /= Large or Back'First /= 5 or Back'Last /= 9004 or
           Null_Back'First /= 8 or Null_Back'Last /= 3 then
            raise Program_Error with "stream result bounds";
        end if;
    end;
    Ada.Streams.Stream_IO.Delete (File);
    Put_Line ("stream results ok");
end Streamresults;
