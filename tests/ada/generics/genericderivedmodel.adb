package body GenericDerivedModel is
    function Read (X : Root) return Integer is
    begin
        return Integer (X) + 1;
    end Read;
    function Read (X : Child) return Integer is
    begin
        return Integer (X) + 50;
    end Read;
end GenericDerivedModel;
