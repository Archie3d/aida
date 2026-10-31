package Categorized_Pure is
    pragma Pure;
    pragma Assert (True);
    type Callback is access function (Value : Integer) return Integer;
    type Reference is access all Integer;
    for Reference'Storage_Size use 0;
    type Constant_Reference is access constant Integer;
    Number : constant Constant_Reference := new Integer'(42);
    Value : constant Integer := 25;
    function Compute return Integer;
    type Nonstatic_Default is record
        Item : Integer := Compute;
    end record;
    Explicit_Value : constant Nonstatic_Default := (Item => 31);
    type Static_Default is record
        Item : Integer := 5;
    end record;
    Default_Value : constant Static_Default := (others => <>);
    function Increment (Value : Integer) return Integer;
end Categorized_Pure;
