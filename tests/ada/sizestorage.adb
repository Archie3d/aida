with Ada.Text_IO; use Ada.Text_IO;
with System;
with Size_Storage; use Size_Storage;
procedure SizeStorage is
    function Raw (Address : System.Address; Bytes, Signed : Integer) return Long_Integer;
    pragma Import (C, Raw, "__test_size_value");
    function Offset (First, Second : System.Address) return Long_Integer;
    pragma Import (C, Offset, "__test_size_offset");
    procedure Touch (Address : System.Address; Count : Long_Integer);
    pragma Import (C, Touch, "__test_size_touch");
    procedure Native_Change (Item : in out Roomy);
    pragma Import (C, Native_Change, "__test_size_touch_roomy");
    function Native_Derived (Item : in Derived) return Integer;
    pragma Import (C, Native_Derived, "__test_size_touch_derived");
    procedure Native_Set (Item : out Long_Integer);
    pragma Import (C, Native_Set, "__test_size_set_long");
    procedure Native_Bad (Item : out Long_Integer);
    pragma Import (C, Native_Bad, "__test_size_bad_long");
    subtype Half is Byte;
    for Half'Size use 16;
    type Bytes is array (1 .. 3) of Byte;
    type Guards is array (1 .. 16) of Byte;
    type Compact_Record is record
        First, Second : Byte;
        Tail : Integer;
    end record;
    type Boundary is record
        Item : Pair;
        Guard : Guards;
    end record;
    subtype Plain is Pair;
    for Plain'Size use 64;
    type Colors is (Red, Green, Blue);
    for Colors use (-1000, 250, 65535);
    subtype Green_Only is Colors range Green .. Green;
    for Green_Only'Size use 8;
    subtype Upper is Colors range Green .. Blue;
    for Upper'Size use 16;
    type Modulus is mod 2 ** 64;
    subtype Mod_Byte is Modulus range 0 .. 255;
    for Mod_Byte'Size use 8;
    type Open_Array is array (Integer range <>) of Byte;
    subtype Padded_Array is Open_Array (1 .. 3);
    for Padded_Array'Size use 64;
    type Matrix is array (1 .. 2, 1 .. 3) of Byte;
    for Matrix'Size use 64;
    type Fixed_Padded is array (1 .. 3) of Byte;
    for Fixed_Padded'Size use 64;
    for Fixed_Padded'Alignment use 8;
    type Row_Array is array (1 .. 2) of Padded_Array;
    type Large_Array is array (1 .. 3) of Byte;
    for Large_Array'Size use 560000;
    type Large_Pointer is access Large_Array;
    X : Byte := -100;
    H : Half := -100;
    W : Wide := -100;
    A : Bytes := (-128, 0, 127);
    Packed_Fields : Compact_Record := (-1, 2, 3);
    B : Boundary := (Item => (1, 2), Guard => (others => 85));
    P : Plain := (3, 4);
    R : Roomy := (7, 8);
    D : Derived := (9, 10);
    Small : Short_Array := (others => R);
    G : Green_Only := Green;
    U : Upper := Blue;
    M : Mod_Byte := 255;
    Extended : Modulus := M + 1;
    V : Padded_Array := (1, 2, 3);
    Q : Matrix := ((1, 2, 3), (4, 5, 6));
    F : Fixed_Padded := (1, 2, 3);
    Rows : Row_Array := ((1, 2, 3), (4, 5, 6));
    Large : Large_Array := (1, 2, 3);
    Heap : Large_Pointer := new Large_Array'(4, 5, 6);
    function Ident (Item : Byte) return Byte is
        function Captured return Byte is
        begin
            return Item;
        end Captured;
    begin
        return Captured;
    end Ident;
    procedure Change (Item : in out Roomy; Fail : Boolean := False) is
    begin
        if Item'Size /= 256 then raise Program_Error; end if;
        if Fail then
            Item.A := 33;
            raise Constraint_Error;
        end if;
        Item.A := 22;
    end Change;
    procedure Change_Array (Item : in out Pair_Array) is
    begin
        if Item'Size /= 1024 then raise Program_Error; end if;
        Item (3).A := 99;
    end Change_Array;
    function Make return Roomy is
    begin
        return P;
    end Make;
    procedure Check_Capture is
    begin
        if X /= -100 or W /= -100 then raise Program_Error; end if;
    end Check_Capture;
begin
    Check_Capture;
    Touch (Large'Address, 70000);
    Touch (Heap.all'Address, 70000);
    Change_Array (Small);
    if Small (3).A /= 99 then raise Program_Error; end if;
    if Byte'Size /= 8 or Half'Size /= 16 or Long_Integer'Size /= 64 or Integer'Size /= 32
        or Pair'Size /= 128 or Roomy'Size /= 256 or Plain'Size /= 64
        or Pair_Array'Size /= 1024 or Short_Array'Size /= 768
        or Raw (X'Address, 1, 1) /= -100 or Raw (H'Address, 2, 1) /= -100
        or Raw (W'Address, 8, 1) /= -100 or Raw (G'Address, 1, 0) /= 250
        or Raw (U'Address, 2, 0) /= 65535 or Colors'Pos (U) /= 2
        or Compact_Record'Size /= 64
        or Offset (Packed_Fields.First'Address, Packed_Fields.Second'Address) /= 1
        or Offset (Packed_Fields.First'Address, Packed_Fields.Tail'Address) /= 4
        or Packed_Fields.First /= -1 or Packed_Fields.Second /= 2 or Packed_Fields.Tail /= 3
        or Offset (A (1)'Address, A (2)'Address) /= 1
        or Offset (Small (1)'Address, Small (2)'Address) /= 32
        or Offset (Global (1)'Address, Global (2)'Address) /= 32
        or Offset (Rows (1)'Address, Rows (2)'Address) /= 8
        or F (1 .. 3)'Size /= 24 or F (2 .. 2)'Alignment /= 1
        or Q (2, 3) /= 6 or Heap.all (3) /= 6 or Large (3) /= 3
        or Ident (X) + 5000000000 /= 4999999900 or Extended /= 256 then
        raise Program_Error;
    end if;
    if Copy (D)'Size /= 512 or Native_Derived (Copy (D)) /= 1
        or Native_Derived (Derived (P)) /= 1 then
        raise Program_Error;
    end if;
    Change (P);
    if P.A /= 22 then raise Program_Error; end if;
    begin
        Change (P, True);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if P.A /= 22 then raise Program_Error; end if;
    Native_Change (B.Item);
    if B.Item.A /= 77 or B.Guard /= Guards'(others => 85) then raise Program_Error; end if;
    R := P;
    P := Make;
    if R.A /= 22 or P.A /= 22 then raise Program_Error; end if;
    Native_Set (X);
    if X /= -99 then raise Program_Error; end if;
    begin
        Native_Bad (X);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if X /= -99 then raise Program_Error; end if;
    W := 127;
    begin
        W := W + Integer'Last;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if W /= 127 then raise Program_Error; end if;
    declare
        C : Padded_Array := Open_Array'(V (1 .. 1) & V (2 .. 3));
    begin
        if C (3) /= 3 then raise Program_Error; end if;
    end;
    Put_Line ("Size clauses separate scalar arithmetic and composite storage");
end SizeStorage;
