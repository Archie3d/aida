package GenericDerivedModel is
    type Root is range 0 .. 100;
    function Read (X : Root) return Integer;
    type Child is new Root;
    overriding function Read (X : Child) return Integer;
end GenericDerivedModel;
