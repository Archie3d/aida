package Abstract_Model is
    type Root is abstract tagged record
        Value : Integer := 3;
    end record;
    procedure Change (Self : in out Root) is abstract;
    procedure Via_Alias (Self : in out Root) renames Change;
    function Read (Self : Root) return Integer is abstract;
    function Make return Root is abstract;
    function "+" (Left, Right : Root) return Root is abstract;
    function Default_Read (Self : Root := Make) return Integer;
    function Sum (Self : Root; Other : Root := Make) return Integer;
    function Probe (Self : access Root) return Integer is abstract;
    type Opaque is abstract tagged private;
    package Leaves is
        type Middle is abstract new Root with null record;
        overriding procedure Change (Self : in out Middle) is abstract;
        type Leaf is new Middle with null record;
        overriding procedure Change (Self : in out Leaf);
        overriding procedure Via_Alias (Self : in out Leaf);
        overriding function Read (Self : Leaf) return Integer;
        overriding function Probe (Self : access Leaf) return Integer;
        overriding function Make return Leaf;
        overriding function "+" (Left, Right : Leaf) return Leaf;
        -- An inherited concrete operation remains usable without another body.
        type Last_Leaf is new Leaf with null record;
    end Leaves;
    type Private_Root is abstract tagged private;
    procedure Act (Self : in out Private_Root) is abstract;
private
    type Opaque is tagged null record;
    type Private_Root is abstract tagged null record;
    type Private_Leaf is new Private_Root with null record;
    overriding procedure Act (Self : in out Private_Leaf) is null;
end Abstract_Model;
