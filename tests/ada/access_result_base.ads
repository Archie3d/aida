package Access_Result_Base is
    type Root is abstract tagged record
        Value : Integer := 3;
    end record;
    function Make (Mode : Integer := 0) return access Root is abstract;
    function Identity (Self : access Root; Mode : Integer := 0) return access Root is abstract;
    function Alias_Of (Self : access Root; Mode : Integer := 0) return access Root is abstract;
    function Read (Self : Root) return Integer is abstract;
    procedure Pair (First, Second : access Root; Third : access Root := Make) is null;
    function Sum (Self : Root; Other : access Root := Make) return Integer;
end Access_Result_Base;
