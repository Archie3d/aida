package body GenericInterfaceLibrary is
    function Static_Read (X : T) return Integer is
    begin
        return Read (X);
    end Static_Read;
    function Dynamic_Read (X : F'Class) return Integer is
    begin
        return Read (X);
    end Dynamic_Read;
end GenericInterfaceLibrary;
