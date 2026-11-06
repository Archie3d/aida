generic
    type Element is private;
    Seed : Element;
    with function Next_Value (Value : Element) return Element;
package Generic_Family is
    type Cell is private;
    Fault : exception;
    function Get (Value : Cell) return Element;
    function Count return Integer;
private
    type Cell is record
        Value : Element := Seed;
    end record;
    Touches : Integer := 0;
end Generic_Family;
