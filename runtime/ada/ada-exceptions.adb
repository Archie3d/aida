package body Ada.Exceptions is
    function Message_Length (X : Exception_Occurrence) return Integer;
    pragma Import (C, Message_Length, "__ada_exception_message_length");
    procedure Copy_Message (X : Exception_Occurrence; Target : out String);
    pragma Import (C, Copy_Message, "__ada_exception_message_copy");
    function Information_Length (X : Exception_Occurrence) return Integer;
    pragma Import (C, Information_Length, "__ada_exception_information_length");
    procedure Copy_Information (X : Exception_Occurrence; Target : out String);
    pragma Import (C, Copy_Information, "__ada_exception_information_copy");

    function Exception_Name (X : Exception_Occurrence) return String is
    begin
        return Exception_Name (Exception_Identity (X));
    end Exception_Name;

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
