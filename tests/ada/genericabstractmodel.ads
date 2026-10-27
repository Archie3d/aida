package GenericAbstractModel is
    subtype Offset is Integer range 1 .. 10;
    type Root is abstract tagged record
        Value : Integer := 1;
    end record;
    function Read (Object : Root; Extra : Offset := 1) return Integer is abstract;
    procedure Change (Object : in out Root; Amount : Integer := 1) is abstract;
    function Make (Value : Integer := 2) return Root is abstract;
    function Pointer_Read (Object : access Root) return Integer is abstract;
    type Child is new Root with null record;
    overriding function Read (Hidden : Child; Extra : Offset := 2) return Integer;
    overriding procedure Change (Hidden : in out Child; Amount : Integer := 2);
    overriding function Make (Value : Integer := 3) return Child;
    overriding function Pointer_Read (Object : access Child) return Integer;
end GenericAbstractModel;
