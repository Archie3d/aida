procedure BooleanArrayErrors is
    type Matrix is array (1 .. 2, 1 .. 2) of Boolean;
    type Numbers is array (1 .. 2) of Integer;
    type Bits is array (1 .. 2) of Boolean;
    type Other_Bits is array (1 .. 2) of Boolean;
    M : Matrix := (others => (others => True));
    N : Numbers := (1, 2);
    B : Bits := (True, False);
    O : Other_Bits := (True, False);
    Invalid_Matrix : Matrix := not M;
    Invalid_Component : Numbers := not N;
    Invalid_Types : Bits := B and O;
    Invalid_Short_Circuit : Boolean := B and then B;
begin
    null;
end BooleanArrayErrors;
