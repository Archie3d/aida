generic
    type Index is range <>;
    type Item is private;
    type Vector is array (Index) of Item;
    type Open_Vector is array (Index range <>) of Item;
package GenericArrayConstraintLibrary is
    function Copy (Value : Vector) return Vector;
    function Copy_Open (Value : Open_Vector) return Open_Vector;
    generic
        type Other is array (Index) of Item;
    package Inner is
        function Copy (Value : Other) return Other;
    end Inner;
end GenericArrayConstraintLibrary;
