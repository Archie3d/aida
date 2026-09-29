package Dispatch_Model is
    type Root is tagged record
        X : Integer := 2;
    end record;
    function Value (Item : Root) return Integer;
    type Child is new Root with record
        Y : Integer := 40;
    end record;
    overriding function Value (Item : Child) return Integer;
    function Extra (Item : Child) return Integer;
    type Leaf is new Child with null record;
    overriding function Value (Item : Leaf) return Integer;
end Dispatch_Model;
