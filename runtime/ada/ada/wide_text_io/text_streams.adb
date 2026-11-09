-- Reuse the ordinary locked file-stream adapter and its library-level tag.
with Ada.Streams.Stream_IO;
package body Ada.Wide_Text_IO.Text_Streams is
    function Stream (File : File_Type) return Stream_Access is
        function Native (File : File_Type) return Stream_Access;
        pragma Import (C, Native, "__ada_file_stream_view");
    begin
        return Native (File);
    end Stream;
end Ada.Wide_Text_IO.Text_Streams;
