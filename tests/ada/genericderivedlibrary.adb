package body GenericDerivedLibrary is
    function Read_Ancestor (X : T) return Integer is
    begin
        return Read (X);
    end Read_Ancestor;
end GenericDerivedLibrary;
