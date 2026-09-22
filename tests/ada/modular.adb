with Ada.Text_IO; use Ada.Text_IO;

procedure Modular is
    type Byte is mod 256;
    type Digit is mod 10;
    type Word is mod 2 ** 32;
    type One is mod 1;
    type Nonbinary is mod 4294967295;
    type Octet is mod 256;
    for Octet'Size use 8;
    subtype Small is Byte range 1 .. 3;
    type Derived is new Byte;
    B : Byte := 255;
    D : Digit := 9;
    W : Word := Word'Last;
    Z : One := 0;
    Large : Nonbinary := Nonbinary'Last;
    Packed_Value : Octet := 255;
    C : constant Byte := 255 + 2;
    Product : constant Word := 4294967295 * 4294967295;
    Bits : constant Digit := 8 or 7;
    Complement : constant Digit := not 3;
    Negative : constant Byte := -1;
    Static_Power : constant Byte := 3 ** 20;
    Exponent : Integer := 32;
    Count : Integer := 0;
    type Values is array (Byte range 1 .. 3) of Byte;
    A : Values := (1, 2, 3);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    function Add (X : Byte; Y : Byte) return Byte is
    begin
        return X + Y;
    end Add;
begin
    Check (C = 1 and Product = 1 and Bits = 5 and Complement = 6 and Negative = 255);
    Check (Byte'Modulus = 256 and Word'Modulus = 4294967296);
    Check (Small'Modulus = 256 and Small'Base'First = 0 and Small'Base'Last = 255);
    Check (Byte'First = 0 and Word'Last = 4294967295);
    Check (B + 2 = C and B - 255 = 0 and -B = 1);
    Check (B * B = 1 and B / 2 = 127 and B rem 2 = 1 and B mod 2 = 1);
    Check ((B and 15) = 15 and (B xor 15) = 240 and (B or 0) = 255);
    Check ((not B) = 0 and (+B) = 255);
    Check (D + 2 = 1 and D * D = 1 and -D = 1 and (not D) = 0);
    D := 8;
    Check ((D or 7) = Bits and (D xor 7) = 5 and (D and 7) = 0);
    Check (W * W = Product and W + 1 = 0 and -W = 1);
    Check (W > 2147483647 and W / 2 = 2147483647);
    Check (W ** Exponent = 1 and W ** 0 = 1);
    Check ((not W) = 0 and (W xor 2147483647) = 2147483648);
    Check (Large * Large = 1 and Large + 1 = 0 and Large ** Exponent = 1);
    Check (Static_Power = 145 and (not Digit'(3)) = 6);
    Check (Packed_Value + 1 = 0 and Octet'Size = 8);
    Check (Z + Z = 0 and Z * Z = 0 and Z ** 0 = 0 and (not Z) = 0);
    Check (Add (255, 2) = 1 and Derived (B) + 1 = 0);
    Check (Word (B) = 255 and Long_Integer (W) = 4294967295);
    Check (Byte (2.5) = 3 and Byte'Val (255) = B);
    Check (Byte'Succ (1) = 2 and Byte'Pred (1) = 0);
    Check (Byte'Image (B) = " 255" and Word'Image (W) = " 4294967295");
    for I in Byte range 254 .. 255 loop
        Count := Count + 1;
    end loop;
    for I in reverse Word range 4294967294 .. 4294967295 loop
        Count := Count + 1;
    end loop;
    Check (Count = 4 and A (2) = 2);
    Put_Line ("modular arithmetic passed");
end Modular;
