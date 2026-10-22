with Ada.Text_IO; use Ada.Text_IO;
procedure ModularWide is
    type Word is mod 2 ** 64;
    type Prime is mod 18446744073709551557;
    type Byte is mod 256;
    subtype Upper is Word range 2 ** 63 .. Word'Last;
    X : Word := Word'Last;
    Y : Word := 2 ** 63;
    Lower_Bound : Word := 2 ** 63 - 1;
    Upper_Bound : Word := 2 ** 63 + 1;
    subtype Across is Word range Lower_Bound .. Upper_Bound;
    P : Prime := Prime'Last;
    N : Long_Integer := -1;
    E : Integer := 2;
    Count : Integer := 0;
    C : constant Word := 18446744073709551615 * 18446744073709551615;
    generic
        type T is mod <>;
    function Twice (V : T) return T;
    function Twice (V : T) return T is
    begin
        return V + V;
    end Twice;
    function Double is new Twice (Word);
    procedure Check (B : Boolean) is
    begin
        if not B then
            raise Program_Error;
        end if;
    end Check;
begin
    Check (Word'Modulus = 2 ** 64 and Word'Last = 18446744073709551615);
    Check (Word'Width = 21 and Word'Size = 64);
    Check (X > Y and Y > 0 and X >= Y and Y < X and Y <= X);
    Check (X + 1 = 0 and X * X = C and C = 1 and -X = 1);
    Check (X / 2 = 9223372036854775807 and X rem 2 = 1 and X mod 2 = 1);
    Check (X ** E = 1 and (not X) = 0 and (X xor Y) = 9223372036854775807);
    Check (P * P = 1 and P + 1 = 0 and P ** E = 1);
    Check (Double (Y) = 0 and Word (X) = X);
    Check (Word'Mod (-1) = X and Word'Mod (N) = X and Byte'Mod (X) = 255);
    Check (Upper'Mod (0) = 0 and Word'Mod (2 ** 100) = 0);
    Check (Word'Succ (Y) = Y + 1 and Word'Pred (Y) = Y - 1);
    Check (Word'Image (X) = " 18446744073709551615");
    Check (Upper'Value ("0") = 0 and Word'Pos (Word'Last) = 18446744073709551615);
    Check (Word'Value ("16#FFFF_FFFF_FFFF_FFFF#") = X);
    Check (X in Upper and 0 not in Upper);
    for I in Word range Word'Last - 1 .. Word'Last loop
        Count := Count + 1;
    end loop;
    Check (Count = 2);
    for I in Across loop
        Check (I in Across);
        Count := Count + 1;
    end loop;
    for I in reverse Across loop
        Count := Count + 1;
    end loop;
    Check (Count = 8);
    case X is
        when 0 .. 9223372036854775807 => raise Program_Error;
        when 9223372036854775808 .. 18446744073709551615 => null;
    end case;
    Check (Word'Min (X, Y) = Y and Word'Max (X, Y) = X);
    Check (Word'Succ (Word'Last) = 0 and Word'Pred (0) = Word'Last);
    Check (Upper'Value ("0") = 0 and Word'Pos (Word'Last) = 18446744073709551615);
    Check (Word'Val (18446744073709551615) = X);
    Check (Word (9223372036854775808.0) = Y);
    Put_Line ("wide modular arithmetic passed");
end ModularWide;
