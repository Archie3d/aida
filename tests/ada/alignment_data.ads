package Alignment_Data is
    type Cell is record
        Value : Boolean;
    end record;
    for Cell'Alignment use 8;
    type Cells is array (1 .. 3) of Cell;
    Global : Cells := ((Value => False), (Value => True), (Value => False));
end Alignment_Data;
