package body GenericPartialPackageLibrary is
    function Read return Integer is
    begin
        return Data.Read;
    end Read;
    procedure Reset is
    begin
        Write (Initial);
    end Reset;
    package body Inner is
        function Read return Integer is
        begin
            return Other.Read;
        end Read;
    end Inner;
end GenericPartialPackageLibrary;
