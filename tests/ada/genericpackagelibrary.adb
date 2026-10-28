package body GenericPackageLibrary is
    function Read return Element is
    begin
        return Data.Read;
    end Read;
    procedure Reset is
    begin
        Write (Initial);
    end Reset;
    function Copy (Value : Cell) return Cell is
    begin
        return Helpers.Copy (Value);
    end Copy;
    procedure Fail is
    begin
        raise Failure;
    end Fail;
    package body Inner is
        function Read return Element is
        begin
            return Other.Read;
        end Read;
    end Inner;
end GenericPackageLibrary;
