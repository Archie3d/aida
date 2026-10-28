package body GenericPackageModel is
    function Read return Item is
    begin
        return Stored;
    end Read;
    procedure Write (Value : Item) is
    begin
        Stored := Value;
    end Write;
    package body Helpers is
        function Copy (Value : Cell) return Cell is
        begin
            return Value;
        end Copy;
    end Helpers;
end GenericPackageModel;
