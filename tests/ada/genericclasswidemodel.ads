package GenericClasswideModel is
    type Root is tagged record
        Value : Integer := 1;
    end record;
    function Read (Object : Root) return Integer;
    procedure Set (Object : in out Root; Value : Integer);
    function Make (Value : Integer := 3) return Root;
    function Copy (Object : Root) return Root;
    procedure Combine (Left : Root; Right : Root);
    type Child is new Root with record
        Extra : Integer := 10;
    end record;
    overriding function Read (Hidden_Object : Child) return Integer;
    overriding procedure Set (Object : in out Child; Value : Integer);
    overriding function Make (Hidden_Value : Integer := 4) return Child;
    overriding function Copy (Object : Child) return Child;
end GenericClasswideModel;
