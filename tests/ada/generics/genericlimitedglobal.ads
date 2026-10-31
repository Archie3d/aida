with Return_Model; use Return_Model;
with GenericLimitedLibrary;
with GenericLimitedArrayLibrary;
package GenericLimitedGlobal is
    function Valid (Value : Cell) return Boolean;
    package Instance is new GenericLimitedLibrary (Cell, Forward, Valid);
    package Array_Instance is new GenericLimitedArrayLibrary (Many (3));
end GenericLimitedGlobal;
