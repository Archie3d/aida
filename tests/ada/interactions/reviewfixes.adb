procedure Reviewfixes is
    type Small is range -128 .. 127;
    for Small'Size use 8;
    Count : Natural := 0;
    type Word is mod 2 ** 64;
    type Words is array (Positive range <>) of Word;
    High : Word := Word'Last;
    Low : Word := 0;
    Middle : Word := 2 ** 63;
    A : Words (1 .. 1) := (1 => High);
    B : Words (1 .. 1) := (1 => Low);
    C : Words (1 .. 1) := (1 => Middle);
    Positive_Zero : Float := 0.0;
    Negative_Zero : Float := Float'Copy_Sign (0.0, -1.0);
    Long_Positive_Zero : Long_Float := 0.0;
    Long_Negative_Zero : Long_Float := Long_Float'Copy_Sign (0.0, -1.0);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    for I in Small range 256 .. 0 loop
        Count := Count + 1;
    end loop;
    for I in reverse Small range 0 .. -256 loop
        Count := Count + 1;
    end loop;
    Check (Count = 0);
    for I in Small range 127 .. 127 loop
        Check (I = 127);
        Count := Count + 1;
    end loop;
    Check (Count = 1);
    for I in reverse Small range -128 .. -128 loop
        Check (I = -128);
        Count := Count + 1;
    end loop;
    Check (Count = 2);
    Check (A > B and A >= B and B < A and B <= A);
    Check (not (A < B) and not (B > A));
    Check ((A > B) = (High > Low));
    Check ((B < C) = (Low < Middle));
    Check ((C > B) = (Middle > Low));
    Check ((A > C) = (High > Middle));
    Check (Float'Copy_Sign (1.0, -Positive_Zero) = -1.0);
    Check (Float'Copy_Sign (1.0, -Negative_Zero) = 1.0);
    Check (Float'Copy_Sign (1.0, abs Positive_Zero) = 1.0);
    Check (Float'Copy_Sign (1.0, abs Negative_Zero) = 1.0);
    Check (Long_Float'Copy_Sign (1.0, -Long_Positive_Zero) = -1.0);
    Check (Long_Float'Copy_Sign (1.0, -Long_Negative_Zero) = 1.0);
    Check (Long_Float'Copy_Sign (1.0, abs Long_Positive_Zero) = 1.0);
    Check (Long_Float'Copy_Sign (1.0, abs Long_Negative_Zero) = 1.0);
end Reviewfixes;
