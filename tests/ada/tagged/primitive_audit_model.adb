package body Primitive_Audit_Model is
    function Make return Root is
    begin
        return (Value => 2);
    end Make;
    not overriding function Read (X : Root) return Integer is
    begin
        return X.Value;
    end Read;
    function Combine (X : Root; Y : Root := Make) return Integer is
    begin
        return X.Value + Y.Value;
    end Combine;
    function Make return Child is
    begin
        return (Root'(Value => 7) with null record);
    end Make;
    function Read (X : Child) return Integer is
    begin
        return X.Value + 10;
    end Read;
    function Combine (X : Child; Y : Child := Make) return Integer is
    begin
        return X.Value + Y.Value + 100;
    end Combine;
    function Make_Wide return Root'Class is
    begin
        return Child'(Make);
    end Make_Wide;
end Primitive_Audit_Model;
