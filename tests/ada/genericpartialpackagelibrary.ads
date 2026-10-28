with GenericPackageModel;
generic
    with package Data is new GenericPackageModel (Item => Integer, others => <>);
    use Data;
package GenericPartialPackageLibrary is
    function Read return Integer;
    procedure Reset;
    generic
        with package Other is new GenericPackageModel (Item => Integer, Initial => Data.Initial);
    package Inner is
        function Read return Integer;
    end Inner;
end GenericPartialPackageLibrary;
