with Ada.Streams.Stream_IO;
with Ada.Text_IO;
with Scalar_Dynamic_Model; use Scalar_Dynamic_Model;
procedure ScalarStreams is
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    Value : Small := 3;
    Real_Value : Real_Range := 1.5;
    Plain : Integer := 0;
    Caught : Integer := 0;
    type Choice is (A, Longest_Name, B);
    for Choice use (A => -10, Longest_Name => 20, B => 70);
    Last : Choice := Longest_Name;
    subtype Choices is Choice range A .. Last;
    Selected : Choices := A;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "scalarstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Integer'Output (Channel, 13);
    Integer'Output (Channel, 13);
    Integer'Output (Channel, 13);
    Small'Output (Channel, 4);
    Long_Float'Output (Channel, 5.0);
    Long_Float'Output (Channel, 5.0);
    Real_Range'Output (Channel, 2.0);
    Fixed_Range'Output (Channel, 1.0);
    Choice'Output (Channel, B);
    Choices'Output (Channel, Longest_Name);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "scalarstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    -- Read checks the actual object's nominal subtype; Input returns the
    -- prefix subtype. Failed reads must leave the actual object unchanged.
    Small'Read (Channel, Plain);
    if Plain /= 13 then raise Program_Error; end if;
    begin Small'Read (Channel, Value); exception when Constraint_Error => Caught := Caught + 1; end;
    if Value /= 3 then raise Program_Error; end if;
    begin Plain := Small'Input (Channel); exception when Constraint_Error => Caught := Caught + 1; end;
    if Plain /= 13 then raise Program_Error; end if;
    Value := Small'Input (Channel);
    begin Real_Range'Read (Channel, Real_Value); exception when Constraint_Error => Caught := Caught + 1; end;
    begin Real_Value := Real_Range'Input (Channel); exception when Constraint_Error => Caught := Caught + 1; end;
    if Real_Value /= 1.5 or else Real_Range'Input (Channel) /= 2.0
        or else Fixed_Range'Input (Channel) /= 1.0 or else Value /= 4 or else Caught /= 4 then
        raise Program_Error;
    end if;
    begin Selected := Choices'Input (Channel); exception when Constraint_Error => Caught := Caught + 1; end;
    if Selected /= A or else Choices'Input (Channel) /= Longest_Name or else Choices'Width /= 12
        or else Caught /= 5 then raise Program_Error; end if;
    Ada.Streams.Stream_IO.Close (File);
    Ada.Text_IO.Put_Line ("dynamic scalar streams");
end ScalarStreams;
