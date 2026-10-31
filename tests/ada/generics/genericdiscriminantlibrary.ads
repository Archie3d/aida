generic
    type Index is range <>;
    type T (First, Last : Index) is limited private;
package GenericDiscriminantLibrary is
    subtype Selected is T (Last => 7, First => 2);
    function Score (X : T) return Index;
    function Matches (X : T) return Boolean;
end GenericDiscriminantLibrary;
