package Rejection_Model is
    type Name is limited private;
    type Item (Length : Positive) is private;
    function Create (Length : Positive) return Item;
    function Check return Boolean;
    type Secret is private;
    function Make_Secret return Secret;
    type Tagged_Item is tagged record
        Value : Integer := 0;
    end record;
    function "+" (Left : Tagged_Item; Right : Integer) return Tagged_Item;
    type Integer_Array is array (Positive range <>) of Integer;
    function Values return Integer_Array;
private
    type Secret (Key : Integer := 7) is record
        Value : Integer := 1;
    end record;
    type Name is array (1 .. 4) of Character;
    type Item (Length : Positive) is record
        Hidden : Integer := 123;
    end record;
end Rejection_Model;
