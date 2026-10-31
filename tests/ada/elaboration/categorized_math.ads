generic
    with function Plus (Left, Right : Integer) return Integer;
package Categorized_Math is
    pragma Preelaborate;
    Value : constant Integer := Plus (20, 22);
end Categorized_Math;
