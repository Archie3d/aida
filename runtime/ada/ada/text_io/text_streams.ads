with Ada.Streams;
package Ada.Text_IO.Text_Streams is
    type Stream_Access is access all Ada.Streams.Root_Stream_Type'Class;
    function Stream (File : File_Type) return Stream_Access;
end Ada.Text_IO.Text_Streams;
