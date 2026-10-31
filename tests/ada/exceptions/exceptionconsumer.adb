with Exception_Partition;
with Ada.Exceptions; use Ada.Exceptions;
with Ada.Streams.Stream_IO;
procedure ExceptionConsumer is
    File : Ada.Streams.Stream_IO.File_Type;
begin
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "exceptionpartition.tmp");
    declare
        Saved : Exception_Occurrence := Exception_Occurrence'Input (Ada.Streams.Stream_IO.Stream (File));
    begin
        Ada.Streams.Stream_IO.Delete (File);
        if Exception_Identity (Saved) /= Exception_Partition.Failure'Identity
            or Exception_Message (Saved) /= Exception_Partition.Message then
            raise Program_Error;
        end if;
        begin
            Reraise_Occurrence (Saved);
            raise Program_Error;
        exception
            when Exception_Partition.Failure => null;
        end;
    end;
end ExceptionConsumer;
