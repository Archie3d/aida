package body Ada.Storage_IO is
    -- Compiler operations stage a native representation in aligned storage.
    -- Read then uses ordinary Ada assignment, including Adjust/Finalize.
    procedure Read_Value (Buffer : in Buffer_Type; Item : out Element_Type);
    pragma Import (C, Read_Value, "__ada_storage_io_read");
    procedure Write_Value (Buffer : out Buffer_Type; Item : in Element_Type);
    pragma Import (C, Write_Value, "__ada_storage_io_write");

    procedure Read (Buffer : in Buffer_Type; Item : out Element_Type) is
    begin
        Read_Value (Buffer, Item);
    exception
        when Constraint_Error => raise Data_Error;
    end Read;

    procedure Write (Buffer : out Buffer_Type; Item : in Element_Type) is
    begin
        Write_Value (Buffer, Item);
    end Write;
end Ada.Storage_IO;
