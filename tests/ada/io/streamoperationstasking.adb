with Ada.Streams;
with Ada.Streams.Stream_IO;
with Ada.Text_IO;
procedure StreamOperationsTasking is
    package IO renames Ada.Streams.Stream_IO;
    use Ada.Streams;
    use type IO.Count;
    use type IO.Stream_Access;
    File : IO.File_Type;
    Channel : IO.Stream_Access;
    procedure Write_Region (Number : Positive) is
        Data : Stream_Element_Array (1 .. 16) := (others => Stream_Element (Number));
    begin
        for Iteration in 1 .. 100 loop
            IO.Write (File, Data, To => IO.Positive_Count ((Number - 1) * 16 + 1));
            IO.Flush (File);
            if IO.Stream (File) /= Channel then raise Program_Error; end if;
        end loop;
    end Write_Region;
    procedure Read_Region (Number : Positive) is
        Data : Stream_Element_Array (7 .. 22);
        Last : Stream_Element_Offset;
    begin
        for Iteration in 1 .. 100 loop
            IO.Read (File, Data, Last, From => IO.Positive_Count ((Number - 1) * 16 + 1));
            if Last /= 22 then raise Program_Error; end if;
            for Index in Data'Range loop
                if Data (Index) /= Stream_Element (Number) then raise Program_Error; end if;
            end loop;
        end loop;
    end Read_Region;
begin
    IO.Create (File);
    Channel := IO.Stream (File);
    declare
        task First;
        task Second;
        task Third;
        task Fourth;
        task body First is begin Write_Region (1); end First;
        task body Second is begin Write_Region (2); end Second;
        task body Third is begin Write_Region (3); end Third;
        task body Fourth is begin Write_Region (4); end Fourth;
    begin
        null;
    end;
    if IO.Size (File) /= 64 then raise Program_Error; end if;
    IO.Set_Mode (File, IO.In_File);
    declare
        task First;
        task Second;
        task Third;
        task Fourth;
        task body First is begin Read_Region (1); end First;
        task body Second is begin Read_Region (2); end Second;
        task body Third is begin Read_Region (3); end Third;
        task body Fourth is begin Read_Region (4); end Fourth;
    begin
        null;
    end;
    IO.Close (File);
    Ada.Text_IO.Put_Line ("concurrent stream operations passed");
end StreamOperationsTasking;
