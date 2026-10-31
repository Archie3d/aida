package Box_Library is
    N : Integer := 3;
    type Link is access Integer;
    type Item is record
        Value : Integer := 42;
        Pointer : Link;
    end record;
    type Items is array (Integer range <>) of Item;
    subtype Row is Items (1 .. N);
    type Holder is record
        Data : Row;
        Text : String (1 .. N) := (others => 'x');
    end record;
    A : Holder := (others => <>);
    B : Row := (others => <>);
    function Make return Holder;
end Box_Library;
