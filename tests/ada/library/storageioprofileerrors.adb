with System.Storage_Elements; use System.Storage_Elements;
procedure StorageIOProfileErrors is
    procedure Broken;
    pragma Import (C, Broken, "__ada_storage_io_read");
    procedure Wrong_Size (Buffer : in Storage_Array; Item : out Integer);
    pragma Import (C, Wrong_Size, "__ada_storage_io_read");
begin
    null;
end StorageIOProfileErrors;
