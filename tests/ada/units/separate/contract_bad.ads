generic
    type Item is private;
package Contract_Bad is
    function Add (L, R : Item) return Item;
end Contract_Bad;
