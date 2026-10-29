package body Ada.Streams.Stream_IO is
    overriding procedure Read (Stream : in out File_Stream;
                               Item : out Stream_Element_Array;
                               Last : out Stream_Element_Offset) is
        procedure Read_Bytes (Handle : System.Address; Item : out Stream_Element_Array;
                              First : Stream_Element_Offset; Last : out Stream_Element_Offset);
        pragma Import (C, Read_Bytes, "__ada_file_stream_elements_read");
    begin
        Read_Bytes (Stream.Handle, Item, Item'First, Last);
    end Read;
    overriding procedure Write (Stream : in out File_Stream;
                                Item : Stream_Element_Array) is
        procedure Write_Bytes (Handle : System.Address; Item : Stream_Element_Array);
        pragma Import (C, Write_Bytes, "__ada_file_stream_elements_write");
    begin
        Write_Bytes (Stream.Handle, Item);
    end Write;

    procedure Read (File : in File_Type;
                    Item : out Stream_Element_Array;
                    Last : out Stream_Element_Offset) is
        procedure Read_Elements (File : in File_Type;
                                 Item : out Stream_Element_Array;
                                 First : in Stream_Element_Offset;
                                 Last : out Stream_Element_Offset);
        pragma Import (C, Read_Elements, "__ada_stream_elements_read");
    begin
        Read_Elements (File, Item, Item'First, Last);
    end Read;
end Ada.Streams.Stream_IO;
