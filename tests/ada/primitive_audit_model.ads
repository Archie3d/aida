package Primitive_Audit_Model is
    type Root is tagged record
        Value : Integer;
    end record;
    function Make return Root;
    not overriding function Read (X : Root) return Integer;
    function Combine (X : Root; Y : Root := Make) return Integer;
    type Child is tagged private;
    overriding function Make return Child;
    overriding function Read (X : Child) return Integer;
    function Make_Wide return Root'Class;
private
    type Child is new Root with null record;
    overriding function Combine (X : Child; Y : Child := Make) return Integer;
end Primitive_Audit_Model;
