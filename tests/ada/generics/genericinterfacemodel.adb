package body GenericInterfaceModel is
    function Read (X : Item) return Integer is
    begin
        return X.Value;
    end Read;
end GenericInterfaceModel;
