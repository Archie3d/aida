generic
    type Item is private;
    Initial : Item;
package GenericPackageModel is
    subtype Element is Item;
    type Cell is record
        Value : Item;
    end record;
    Failure : exception;
    function Read return Item;
    procedure Write (Value : Item);
    package Helpers is
        function Copy (Value : Cell) return Cell;
    end Helpers;
private
    Stored : Item := Initial;
end GenericPackageModel;
