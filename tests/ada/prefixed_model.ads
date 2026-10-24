package Prefixed_Model is
    type Root is tagged record
        Value : Integer := 1;
    end record;
    procedure Add (Object : in out Root; Amount : Integer := 1);
    procedure Add (Object : in out Root; Twice : Boolean);
    function Read (Object : Root) return Integer;
    function Read (Object : Root) return Boolean;
    function Via_Access (Object : access constant Root) return Integer;
    procedure Set (Object : access Root; Value : Integer);
    function Copy (Object : Root) return Root;
    function Wide_Read (Object : Root'Class; Amount : Integer := 0) return Integer;
    procedure Second (Value : Integer; Object : Root);
    type Child is new Root with record
        Extra : Integer := 10;
    end record;
    overriding function Read (Object : Child) return Integer;
    overriding function Via_Access (Object : access constant Child) return Integer;
    overriding function Copy (Object : Child) return Child;
    type Root_Pointer is access all Root;
    procedure Named_First (Object : Root_Pointer);
    type Other is tagged record
        Value : Integer := 77;
    end record;
    function Read (Object : Other) return Character;
    type Private_Root is tagged private;
    function Read (Object : Private_Root) return Integer;
private
    type Private_Root is tagged record
        Value : Integer := 42;
    end record;
    function Hidden (Object : Private_Root) return Integer;
end Prefixed_Model;
