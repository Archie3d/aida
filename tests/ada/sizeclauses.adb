with Ada.Text_IO; use Ada.Text_IO;

procedure SizeClauses is
    Byte_Bits : constant := 8;
    type Signed_Byte is range -128 .. 127;
    for Signed_Byte'Size use 8;
    type Unsigned_Byte is range 0 .. 255;
    for Unsigned_Byte'Size use 8;
    type Signed_Half is range -32768 .. 32767;
    for Signed_Half'Size use 16;
    type Unsigned_Half is range 0 .. 65535;
    for Unsigned_Half'Size use Byte_Bits * 2;
    type Signed_Word is range -2147483648 .. 2147483647;
    for Signed_Word'Size use 32;
    type Wide is range -5000000000 .. 5000000000;
    for Wide'Size use 64;
    type Widened is range -128 .. 127;
    for Widened'Size use 64;
    type Byte_Mod is mod 256;
    for Byte_Mod'Size use 8;
    type Half_Mod is mod 65536;
    for Half_Mod'Size use 16;
    type Word_Mod is mod 2 ** 31;
    for Word_Mod'Size use 32;
    type Wide_Mod is mod 2 ** 32;
    for Wide_Mod'Size use 64;
    type Color is (Red, Green, Blue);
    for Color'Size use 8;
    type Half_Color is (Red, Green, Blue);
    for Half_Color'Size use 16;
    type Word_Color is (Red, Green, Blue);
    for Word_Color'Size use 32;
    type Single is digits 6;
    for Single'Size use 32;
    type Double is digits 15;
    for Double'Size use 64;
    type Fixed is delta 0.125 range -10.0 .. 10.0;
    for Fixed'Size use 64;
    type Pointer is access Integer;
    for Pointer'Size use 64;
    subtype Small is Signed_Byte range -10 .. 10;
    for Small'Size use 8;
    type Derived is new Signed_Word;
    for Derived'Size use 64;

    type Bytes is array (1 .. 2) of Signed_Byte;
    for Bytes'Size use 16;
    type Words is array (1 .. 4) of Signed_Word;
    for Words'Size use 128;
    type Pair is record
        A, B : Unsigned_Byte;
    end record;
    for Pair'Size use 16;

    B : Bytes := (-128, 127);
    W : Words := (-2147483648, 2147483647, 1, 2);
    P : Pair := (255, 128);
    H : Signed_Half := -32768;
    U : Unsigned_Half := 65535;
    L : Wide := -5000000000;
    X : Widened := -128;
    M8 : Byte_Mod := 255;
    M16 : Half_Mod := 65535;
    M32 : Word_Mod := 2147483647;
    M64 : Wide_Mod := 4294967295;
    C8 : Color := Blue;
    C16 : Half_Color := Green;
    C32 : Word_Color := Blue;
    F : Single := 1.5;
    D : Double := 2.5;
    Q : Fixed := 0.125;
    Ptr : Pointer := new Integer'(42);
    S : Small := -10;
    R : Derived := -2147483648;
    -- Confirming an existing size does not invalidate prior uses.
    for Signed_Byte'Size use 8;
begin
    if B (1) /= -128 or B (2) /= 127 or P.A /= 255 or P.B /= 128
        or H /= -32768 or U /= 65535 or W (1) /= -2147483648
        or W (2) /= 2147483647 or L /= -5000000000 or X /= -128
        or M8 /= 255 or M16 /= 65535 or M32 /= 2147483647
        or M64 /= 4294967295 or C8 /= Blue or C16 /= Green or C32 /= Blue
        or F /= 1.5 or D /= 2.5 or Q /= 0.125 or Ptr.all /= 42
        or S /= -10 or R /= -2147483648 then
        raise Program_Error;
    end if;
    if Signed_Byte'Size /= 8 or Unsigned_Byte'Size /= 8
        or Signed_Half'Size /= 16 or Unsigned_Half'Size /= 16
        or Signed_Word'Size /= 32 or Wide'Size /= 64 or Widened'Size /= 64
        or Half_Color'Size /= 16 or Word_Color'Size /= 32
        or Bytes'Size /= 16 or Words'Size /= 128 or Pair'Size /= 16
        or Small'Size /= 8 or Derived'Size /= 64 then
        raise Program_Error;
    end if;
    Put_Line ("size clauses preserve values and layouts");
end SizeClauses;
