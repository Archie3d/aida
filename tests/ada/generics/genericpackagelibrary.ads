with GenericPackageModel;
generic
    with package Data is new GenericPackageModel (<>);
    use Data;
package GenericPackageLibrary is
    function Read return Element;
    procedure Reset;
    function Copy (Value : Cell) return Cell;
    procedure Fail;
    generic
        with package Other is new GenericPackageModel (Data.Item, Data.Initial);
    package Inner is
        function Read return Element;
    end Inner;
end GenericPackageLibrary;
