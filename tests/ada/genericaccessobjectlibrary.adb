package body GenericAccessObjectLibrary is
    function Read return Element is
    begin
        return Value.all;
    end Read;
    procedure Set (Replacement : access Element) is
    begin
        Cursor.all := Replacement.all;
    end Set;
    function Read_Other return Element is
    begin
        return Other.all;
    end Read_Other;
end GenericAccessObjectLibrary;
