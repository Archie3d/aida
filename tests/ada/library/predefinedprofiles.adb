with Ada.Text_IO;
with Ada.Streams;
with Ada.Streams.Stream_IO;
with Direct_IO;
with Sequential_IO;
with System;
procedure PredefinedProfiles is
    package S renames Ada.Streams;
    package Files renames S.Stream_IO;
    use type S.Stream_Element;
    use type S.Stream_Element_Offset;
    use type Files.Count;
    procedure Check is
        package First_IO is new Direct_IO (Integer);
        package Second_IO is new Direct_IO (Integer);
        package Sequence_IO is new Sequential_IO (Integer);
        generic
            type Byte is mod <>;
        package Byte_View is
            Modulus : constant := Byte'Modulus;
        end Byte_View;
        package Bytes is new Byte_View (S.Stream_Element);
        function Kind (Value : Integer) return Integer is
        begin
            return 1;
        end Kind;
        function Kind (Value : Files.Count) return Integer is
        begin
            return 2;
        end Kind;
        function Kind (Value : First_IO.Count) return Integer is
        begin
            return 3;
        end Kind;
        function Kind (Value : Second_IO.Count) return Integer is
        begin
            return 4;
        end Kind;
        Half_Tick : constant := System.Tick / 2.0;
        Micros : constant := System.Tick * 1_000_000.0;
        Float_Tick : Float := System.Tick;
        Long_Tick : Long_Float := Half_Tick;
        Fixed_Tick : Duration := System.Tick;
        Element : S.Stream_Element := 255;
        Data : S.Stream_Element_Array (-2 .. -1);
        Last : S.Stream_Element_Offset;
        Binary : Files.File_Type;
        Direct : First_IO.File_Type;
        Sequence : Sequence_IO.File_Type;
        Value : Integer;
    begin
        delay System.Tick;
        delay Half_Tick;
        if Bytes.Modulus /= 256 or S.Stream_Element'Size /= System.Storage_Unit
            or Micros /= 1.0 or Float_Tick /= 0.000_001
            or Long_Tick /= 0.000_000_5 or Fixed_Tick /= Duration (0.000_001)
            or Kind (Integer'(1)) /= 1 or Kind (Files.Count'(1)) /= 2
            or Kind (First_IO.Count'(1)) /= 3 or Kind (Second_IO.Count'(1)) /= 4
        then
            raise Program_Error;
        end if;
        Element := Element + 1;
        if Element /= 0 or (not Element) /= 255 then raise Program_Error; end if;
        Files.Create (Binary);
        Files.Write (Binary, (1 => Element, 2 => 255));
        if Files.Size (Binary) /= 2 or Files.Index (Binary) /= 3 then raise Program_Error; end if;
        Files.Reset (Binary, Files.In_File);
        Files.Read (Binary, Data, Last);
        if Last /= -1 or Data (-2) /= 0 or Data (-1) /= 255 then raise Program_Error; end if;
        Files.Delete (Binary);
        First_IO.Create (Direct);
        First_IO.Write (Direct, 37, 2);
        First_IO.Read (Direct, Value, 2);
        if Value /= 37 or Kind (First_IO.Size (Direct)) /= 3 then raise Program_Error; end if;
        First_IO.Delete (Direct);
        Sequence_IO.Create (Sequence);
        Sequence_IO.Write (Sequence, 53);
        Sequence_IO.Reset (Sequence, Sequence_IO.In_File);
        Sequence_IO.Read (Sequence, Value);
        if Value /= 53 then raise Program_Error; end if;
        Sequence_IO.Delete (Sequence);
    end Check;
    protected Results is
        procedure Passed;
        function Count return Integer;
    private
        Good : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        function Count return Integer is
        begin
            return Good;
        end Count;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
        begin
            for I in 1 .. 10 loop
                Check;
            end loop;
            Results.Passed;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end Exercise;
begin
    Check;
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("predefined profiles passed");
end PredefinedProfiles;
