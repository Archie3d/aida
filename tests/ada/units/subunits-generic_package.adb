separate (Subunits)
package body Generic_Package is
    function Next (Value : Item) return Item is
    begin
        return Value + Item (Bias);
    end Next;
end Generic_Package;
