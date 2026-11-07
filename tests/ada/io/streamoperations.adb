with Ada.Streams;
with Ada.Streams.Stream_IO;
with Ada.Text_IO;
procedure StreamOperations is
    package IO renames Ada.Streams.Stream_IO;
    use Ada.Streams;
    use type IO.Count;
    use type IO.File_Mode;
    use type IO.Stream_Access;
    File : IO.File_Type;
    Channel : IO.Stream_Access;
    Bytes : Stream_Element_Array (5 .. 8) := (10, 20, 30, 40);
    Data : Stream_Element_Array (-3 .. 1) := (others => 99);
    Empty : Stream_Element_Array (8 .. 7);
    Last : Stream_Element_Offset;
    Zero : IO.Count := 0;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
begin
    IO.Create (File);
    Channel := IO.Stream (File);
    IO.Write (File, Bytes);
    Check (IO.Index (File) = 5 and IO.Size (File) = 4);
    IO.Write (File, Empty, To => 3);
    IO.Flush (File);
    Check (IO.Index (File) = 3 and IO.Size (File) = 4);
    IO.Write (File, Bytes (7 .. 8), To => 2);
    Check (IO.Index (File) = 4 and IO.Size (File) = 4);
    IO.Set_Mode (File, IO.In_File);
    Check (IO.Index (File) = 4 and IO.Stream (File) = Channel);
    IO.Read (File, Data, Last, From => 2);
    Check (Last = -1 and Data (-3) = 30 and Data (-2) = 40 and Data (-1) = 40);
    Check (IO.End_Of_File (File) and IO.Index (File) = 5);
    IO.Read (File, Data, Last);
    Check (Last = -4 and IO.Index (File) = 5);
    IO.Read (File, Empty, Last, From => 1);
    Check (Last = 7 and IO.Index (File) = 1 and not IO.End_Of_File (File));
    Check (IO.Size (File) = 4 and IO.Index (File) = 1);
    Ada.Streams.Read (Channel.all, Data (-3 .. -3), Last);
    Check (Last = -3 and Data (-3) = 10 and IO.Index (File) = 2);
    begin
        IO.Flush (File);
        raise Program_Error;
    exception
        when IO.Mode_Error => null;
    end;
    begin
        IO.Write (File, Empty, To => 1);
        raise Program_Error;
    exception
        when IO.Mode_Error => Check (IO.Index (File) = 2);
    end;
    IO.Set_Mode (File, IO.Out_File);
    Check (IO.Index (File) = 2 and IO.Size (File) = 4);
    IO.Write (File, Bytes (5 .. 5));
    IO.Set_Mode (File, IO.Append_File);
    Check (IO.Index (File) = 5);
    IO.Set_Index (File, 2);
    IO.Write (File, Bytes (6 .. 6));
    Check (IO.Index (File) = 3 and IO.Size (File) = 4);
    IO.Write (File, Bytes (7 .. 7), To => 1);
    Check (IO.Index (File) = 2 and IO.Size (File) = 4);
    IO.Set_Mode (File, IO.Append_File);
    IO.Write (File, Bytes (8 .. 8));
    Check (IO.Size (File) = 5 and IO.Index (File) = 6);
    IO.Set_Index (File, 2);
    IO.Reset (File);
    Check (IO.Index (File) = 6);
    IO.Reset (File, IO.Out_File);
    Check (IO.Index (File) = 1 and IO.Size (File) = 5);
    IO.Reset (File);
    Check (IO.Index (File) = 1 and IO.Stream (File) = Channel);
    begin
        IO.Read (File, Data, Last, From => 2);
        raise Program_Error;
    exception
        when IO.Mode_Error => Check (IO.Index (File) = 1);
    end;
    begin
        IO.Write (File, Empty, To => IO.Positive_Count (Zero));
        raise Program_Error;
    exception
        when Constraint_Error => Check (IO.Index (File) = 1);
    end;
    IO.Reset (File, IO.In_File);
    IO.Read (File, Data, Last, From => 9);
    Check (Last = -4 and IO.Index (File) = 9 and IO.End_Of_File (File));
    IO.Set_Index (File, 1);
    IO.Read (File, Data, Last);
    Check (Last = 1 and Data = (30, 20, 40, 40, 40));
    IO.Close (File);
    begin
        IO.Set_Mode (File, IO.In_File);
        raise Program_Error;
    exception
        when IO.Status_Error => null;
    end;
    begin
        IO.Flush (File);
        raise Program_Error;
    exception
        when IO.Status_Error => null;
    end;
    IO.Create (File, IO.Out_File, "streamoperations.tmp");
    IO.Write (File, Bytes);
    IO.Close (File);
    IO.Open (File, IO.Out_File, "streamoperations.tmp");
    Check (IO.Index (File) = 1 and IO.Size (File) = 4);
    IO.Reset (File, IO.In_File);
    IO.Read (File, Data, Last);
    Check (Last = 0 and Data (-3 .. 0) = Bytes);
    IO.Close (File);
    IO.Open (File, IO.Append_File, "streamoperations.tmp");
    Check (IO.Index (File) = 5);
    IO.Delete (File);
    IO.Create (File, IO.Append_File);
    Check (IO.Index (File) = 1 and IO.Size (File) = 0);
    IO.Close (File);
    Ada.Text_IO.Put_Line ("stream operations passed");
end StreamOperations;
