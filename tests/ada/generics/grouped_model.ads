generic
    type Item (<>) is private;
    Left, Right : Item;
    X, Y : in out Integer;
    Step, Extra : in Integer := 1;
    use type Item;
    Agree : Boolean := Left = Right;
package Grouped_Model is
    function Equal return Boolean;
    procedure Update;
end Grouped_Model;
