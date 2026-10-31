package Return_Model is
    type Link is access all Integer;
    type Cell is limited record
        Value : aliased Integer := 0;
        Self : Link;
    end record;
    type Cells is array (Integer range <>) of Cell;
    type Pair is record
        First : Cell;
        Second : Cell;
    end record;
    type Buffer (Length : Natural := 2) is limited record
        Text : String (1 .. Length);
        Item : Cell;
    end record;
    function Make (Value : Integer) return Cell;
    function Forward (Value : Integer) return Cell;
    function Many (Count : Natural) return Cells;
    function Make_Pair return Pair;
    function Make_Buffer (Count : Natural) return Buffer;
end Return_Model;
