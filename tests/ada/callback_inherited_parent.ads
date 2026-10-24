package Callback_Inherited_Parent is
    type Root is tagged record
        Value : Integer := 0;
    end record;
    function Read (Object : Root) return Integer;
    procedure Set (Object : in out Root; Value : Integer);
    function Create (Value : Integer) return Root;
end Callback_Inherited_Parent;
