generic
    type Element is private;
    Value : access Element;
    Cursor : in out access Element;
package GenericAccessObjectLibrary is
    function Read return Element;
    procedure Set (Replacement : access Element);
    generic
        Other : access Element := Value;
    function Read_Other return Element;
end GenericAccessObjectLibrary;
