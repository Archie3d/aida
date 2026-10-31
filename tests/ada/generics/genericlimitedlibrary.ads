generic
    type Item is limited private;
    with function Build (N : Integer) return Item;
    with function Valid (Value : Item) return Boolean;
    Value : Item := Build (31);
package GenericLimitedLibrary is
    function Test return Boolean;
    generic
        Other : Item := Build (31);
    function Test_Other return Boolean;
end GenericLimitedLibrary;
