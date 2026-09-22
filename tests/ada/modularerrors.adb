procedure ModularErrors is
    N : Integer := 10;
    type Dynamic_Modulus is mod N;
    type Real_Modulus is mod 10.0;
    type Zero_Modulus is mod 0;
    type Negative_Modulus is mod -10;
    type Large_Modulus is mod 2 ** 32 + 1;
    type Byte is mod 256;
    type Other is mod 256;
    type Word is mod 2 ** 32;
    B : Byte := 1;
    O : Other := 1;
    I : Integer := 1;
    for Word'Size use 32;
begin
    B := abs B;
    B := B + O;
    B := B and O;
    B := B and then B;
    I := Integer'Modulus;
    I := B'Modulus;
    I := Byte'Modulus (1);
end ModularErrors;
