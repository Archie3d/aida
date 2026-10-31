with Exception_Partition;
with Ada.Exceptions;
with Ada.Streams.Stream_IO;
procedure ExceptionProducer is
    File : Ada.Streams.Stream_IO.File_Type;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "exceptionpartition.tmp");
    begin
        raise Exception_Partition.Failure with Exception_Partition.Message;
    exception
        when E : others =>
            Ada.Exceptions.Exception_Occurrence'Output (Ada.Streams.Stream_IO.Stream (File), E);
    end;
    Ada.Streams.Stream_IO.Close (File);
end ExceptionProducer;
