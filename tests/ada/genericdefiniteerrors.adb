procedure GenericDefiniteErrors is
    A : Boolean := String'Definite;
    X : Integer := 1;
    B : Boolean := X'Definite;
    generic
        type T is private;
    package Definite_Formal is
        Value : Boolean := T'Definite;
    end Definite_Formal;
    generic
        type T (<>) is private;
    package Indefinite_Formal is
        With_Argument : Boolean := T'Definite (1);
        procedure Check (Flag : Boolean);
    end Indefinite_Formal;
    package body Indefinite_Formal is
        procedure Check (Flag : Boolean) is
        begin
            case Flag is
                when T'Definite => null;
                when others => null;
            end case;
        end Check;
    end Indefinite_Formal;
begin
    null;
end GenericDefiniteErrors;
