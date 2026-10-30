package body Ada.Exceptions is
    function Message_Length (X : Exception_Occurrence) return Integer;
    pragma Import (C, Message_Length, "__ada_exception_message_length");
    procedure Copy_Message (X : Exception_Occurrence; Target : out String);
    pragma Import (C, Copy_Message, "__ada_exception_message_copy");
    function Information_Length (X : Exception_Occurrence) return Integer;
    pragma Import (C, Information_Length, "__ada_exception_information_length");
    procedure Copy_Information (X : Exception_Occurrence; Target : out String);
    pragma Import (C, Copy_Information, "__ada_exception_information_copy");

    function Identity_From_Name (Name : String) return Exception_Id;
    pragma Import (C, Identity_From_Name, "__ada_exception_from_name");
    procedure Restore (Target : out Exception_Occurrence; Identity : Exception_Id; Message : String);
    pragma Import (C, Restore, "__ada_occurrence_restore");

    procedure Read_Exception_Occurrence
        (Stream : not null access Ada.Streams.Root_Stream_Type'Class;
         Item : out Exception_Occurrence) is
        Name : constant String := String'Input (Stream);
        Message : constant String := String'Input (Stream);
    begin
        Restore (Item, Identity_From_Name (Name), Message);
    end Read_Exception_Occurrence;

    procedure Write_Exception_Occurrence
        (Stream : not null access Ada.Streams.Root_Stream_Type'Class;
         Item : Exception_Occurrence) is
    begin
        if Exception_Identity (Item) = Null_Id then
            String'Output (Stream, "");
            String'Output (Stream, "");
        else
            String'Output (Stream, Exception_Name (Item));
            String'Output (Stream, Exception_Message (Item));
        end if;
    end Write_Exception_Occurrence;

    function Exception_Name (X : Exception_Occurrence) return String is
    begin
        return Exception_Name (Exception_Identity (X));
    end Exception_Name;

    function Wide_Exception_Name (Id : Exception_Id) return Wide_String is
        Name : constant String := Exception_Name (Id);
        Result : Wide_String (1 .. Name'Length);
    begin
        for I in Name'Range loop
            Result (I) := Wide_Character'Val (Character'Pos (Name (I)));
        end loop;
        return Result;
    end Wide_Exception_Name;

    function Wide_Exception_Name (X : Exception_Occurrence) return Wide_String is
    begin
        return Wide_Exception_Name (Exception_Identity (X));
    end Wide_Exception_Name;

    function Wide_Wide_Exception_Name (Id : Exception_Id) return Wide_Wide_String is
        Name : constant String := Exception_Name (Id);
        Result : Wide_Wide_String (1 .. Name'Length);
    begin
        for I in Name'Range loop
            Result (I) := Wide_Wide_Character'Val (Character'Pos (Name (I)));
        end loop;
        return Result;
    end Wide_Wide_Exception_Name;

    function Wide_Wide_Exception_Name (X : Exception_Occurrence) return Wide_Wide_String is
    begin
        return Wide_Wide_Exception_Name (Exception_Identity (X));
    end Wide_Wide_Exception_Name;

    function Exception_Message (X : Exception_Occurrence) return String is
        Result : String (1 .. Message_Length (X));
    begin
        Copy_Message (X, Result);
        return Result;
    end Exception_Message;

    function Exception_Information (X : Exception_Occurrence) return String is
        Result : String (1 .. Information_Length (X));
    begin
        Copy_Information (X, Result);
        return Result;
    end Exception_Information;
end Ada.Exceptions;
