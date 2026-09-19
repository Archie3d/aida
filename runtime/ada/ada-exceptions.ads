with System;

package Ada.Exceptions is
    type Exception_Id is private;
    Null_Id : constant Exception_Id;
    type Exception_Occurrence is limited private;
    type Exception_Occurrence_Access is access Exception_Occurrence;
    Null_Occurrence : constant Exception_Occurrence;

    function Exception_Identity (X : Exception_Occurrence) return Exception_Id;
    pragma Import (C, Exception_Identity, "__ada_exception_identity");
    function Exception_Name (Id : Exception_Id) return String;
    pragma Import (C, Exception_Name, "__ada_exception_name");
    function Exception_Name (X : Exception_Occurrence) return String;
    function Exception_Message (X : Exception_Occurrence) return String;
    function Exception_Information (X : Exception_Occurrence) return String;

    procedure Raise_Exception (E : Exception_Id; Message : String := "");
    pragma Import (C, Raise_Exception, "__ada_raise_message");
    procedure Reraise_Occurrence (X : Exception_Occurrence);
    pragma Import (C, Reraise_Occurrence, "__ada_reraise");
    procedure Save_Occurrence (Target : out Exception_Occurrence; Source : Exception_Occurrence);
    pragma Import (C, Save_Occurrence, "__ada_save_occurrence");
    function Save_Occurrence (Source : Exception_Occurrence) return Exception_Occurrence_Access;
    pragma Import (C, Save_Occurrence, "__ada_save_occurrence_new");
private
    type Exception_Id is access Integer;
    Null_Id : constant Exception_Id := null;
    type Exception_Occurrence is record
        Identity : Exception_Id := Null_Id;
        Message_Data : System.Address := null;
        Message_Length : Integer := 0;
        -- The procedure preserves up to 200 bytes without allocating; the
        -- function preserves the full message in its returned allocation.
        Saved_Message : String (1 .. 200) := (others => Character'Val (0));
    end record;
    Null_Occurrence : constant Exception_Occurrence :=
        (Null_Id, null, 0, (others => Character'Val (0)));
end Ada.Exceptions;
