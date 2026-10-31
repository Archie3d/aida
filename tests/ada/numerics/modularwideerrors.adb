procedure ModularWideErrors is
    type Word is mod 2 ** 64;
    type Byte is mod 256;
    generic
        type T is mod <>;
    package G is
    end G;
    generic
        type T is range <>;
    package H is
    end H;
    package Bad_Modular is new G (Integer);
    package Bad_Signed is new H (Word);
    X : Word := 0;
    I : Integer;
    F : Long_Float := 1.0;
    type Bad_Index is array (Word range <>) of Integer;
begin
    X := Integer'Mod (1);
    X := X'Mod (1);
    X := Word'Mod;
    X := Word'Mod (1, 2);
    X := Word'Mod (F);
    I := Integer (Word'Last);
    X := Word (-1);
end ModularWideErrors;
