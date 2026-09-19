package body Ada.Exceptions is
    function Message_Length (X : Exception_Occurrence) return Integer;
    pragma Import (C, Message_Length, "__ada_exception_message_length");
    procedure Copy_Message (X : Exception_Occurrence; Target : out String);
    pragma Import (C, Copy_Message, "__ada_exception_message_copy");

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

    -- Information is deliberately limited to the name and optional message;
    -- no source location or traceback is recorded by this runtime yet.
    function Exception_Information (X : Exception_Occurrence) return String is
        Message : String := Exception_Message (X);
    begin
        if Message'Length = 0 then
            return Exception_Name (X);
        end if;
        return Exception_Name (X) & ": " & Message;
    end Exception_Information;
end Ada.Exceptions;
