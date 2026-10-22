with Ada.Text_IO; use Ada.Text_IO;
with System;
with Enum_Storage;
procedure EnumRepresentation is
    function Raw (Address : System.Address; Bytes, Signed : Integer) return Integer;
    pragma Import (C, Raw, "__test_enum_raw");
    Compact : Enum_Storage.Compact_Code := Enum_Storage.Last;
    Wide_Code : Enum_Storage.Wide_Code := Enum_Storage.Last;
    type Color is (Red, Green, Blue);
    for Color use (Blue => 250, Red => -7, Green => 11);
    subtype Upper is Color range Green .. Blue;
    type Other is new Color;
    for Other use (1000, 2000, 3000);
    type Inherited is new Color;
    type Signed_Byte is (Low, Middle, High);
    for Signed_Byte use (-128, 0, 127);
    type Unsigned_Byte is (Low, Middle, High);
    for Unsigned_Byte use (7, 128, 255);
    type Unsigned_Half is (Low, Middle, High);
    for Unsigned_Half use (256, 32768, 65535);
    type Word is (Low, Middle, High);
    for Word use (-2147483648, 0, 2147483647);
    type Widened is (Low, Middle, High);
    for Widened use (-7, 11, 250);
    for Widened'Size use 32;
    type Sized_First is (Low, Middle, High);
    for Sized_First'Size use 32;
    for Sized_First use (7, 11, 250);
    type With_Others is (Low, Middle, High);
    for With_Others use (Low => 5, High => 30, others => 20);
    type Positional_Others is (Low, Middle, High);
    for Positional_Others use (5, 10, others => 20);
    type Letters is ('a', 'A', Name);
    for Letters use (Name => 90, 'A' => 30, 'a' => 10);
    type Singleton is (Only);
    for Singleton use (Only => 100);
    type Color_Array is array (Color) of Color;
    type Variant (Key : Color) is record
        case Key is
            when Red => Number : Integer;
            when Green | Blue => Flag : Boolean;
        end case;
    end record;
    type Pointer is access Color;
    X : Color := Red;
    Y : Other := Green;
    Z : Inherited := Blue;
    S : Signed_Byte := Low;
    U : Unsigned_Byte := High;
    H : Unsigned_Half := High;
    W : Word := Low;
    Wide : Widened := High;
    Sized : Sized_First := High;
    Positional_Item : Positional_Others := High;
    Letter : Letters := 'A';
    A : Color_Array := (Blue, Red, Green);
    V : Variant (Green) := (Key => Green, Flag => True);
    Heap : Pointer := new Color'(Green);
    Count : Integer := 0;
    function Echo (Item : Color) return Color;
    pragma Import (C, Echo, "__test_enum_echo");
    function Code_Of (Item : Color) return Integer;
    pragma Import (C, Code_Of, "__test_enum_echo");
    function Position_Of (Item : Color) return Integer;
    pragma Import (Ada, Position_Of, "__test_enum_echo");
    function Make_Blue return Color;
    pragma Import (C, Make_Blue, "__test_enum_blue");
    procedure Set_Blue (Item : out Color);
    pragma Import (C, Set_Blue, "__test_enum_set_blue");
    procedure Invalid (Item : in out Color);
    pragma Import (C, Invalid, "__test_enum_invalid");
    function Invalid_Result return Color;
    pragma Import (C, Invalid_Result, "__test_enum_invalid_result");
    function Ident (Item : Color) return Color is
        function Captured return Color is
        begin
            return Item;
        end Captured;
    begin
        return Captured;
    end Ident;
    procedure Rotate (Item : in out Color) is
    begin
        Item := Color'Succ (Item);
    end Rotate;
    generic
        type Enum is (<>);
    procedure Swap (Left, Right : in out Enum);
    procedure Swap (Left, Right : in out Enum) is
        Temp : Enum := Left;
    begin
        Left := Right;
        Right := Temp;
    end Swap;
    procedure Swap_Color is new Swap (Color);
begin
    if Enum_Storage.Copy (Wide_Code)'Size /= 32 or Enum_Storage.Copy (Wide_Code)'Alignment /= 4
        or Raw (Wide_Code'Address, 4, 1) /= 250 then
        raise Program_Error;
    end if;
    if Enum_Storage.Copy (Compact)'Size /= 8 or Enum_Storage.Copy (Compact)'Alignment /= 1
        or Raw (Compact'Address, 1, 0) /= 255 or Enum_Storage.Copy (Compact) /= Compact then
        raise Program_Error;
    end if;
    if Raw (X'Address, 2, 1) /= -7 or Raw (Y'Address, 2, 0) /= 2000
        or Raw (Z'Address, 2, 1) /= 250 or Raw (S'Address, 1, 1) /= -128
        or Raw (U'Address, 1, 0) /= 255 or Raw (H'Address, 2, 0) /= 65535
        or Raw (W'Address, 4, 1) /= -2147483648
        or Raw (Wide'Address, 4, 1) /= 250 or Raw (Sized'Address, 4, 1) /= 250
        or Raw (Letter'Address, 1, 0) /= 30 or Raw (Positional_Item'Address, 1, 0) /= 20
        or Raw (Heap.all'Address, 2, 1) /= 11
        or Raw (Enum_Storage.Global'Address, 1, 0) /= 70 then
        raise Program_Error;
    end if;
    if Ident (X) /= Red or Echo (X) /= Red or Color (Y) /= Green
        or Code_Of (X) /= -7 or Position_Of (X) /= 0 or Make_Blue /= Color'(Blue)
        or Other (Color'(Blue)) /= Other'(Blue) or Color'Pos (X) /= 0 or Color'Val (2) /= Blue
        or Color'Pred (Blue) /= Green or not (Color'(Red) < Green and Color'(Green) < Blue)
        or Color'Image (Green) /= "GREEN" or Color'Value ("blue") /= Blue
        or Letters'Pos ('a') /= 0 or Letters'Pos ('A') /= 1
        or Letters'Image (Letter) /= "'A'" or Letters'Value ("'a'") /= 'a'
        or A (Red) /= Blue or A (Blue) /= Green or not V.Flag then
        raise Program_Error;
    end if;
    for I in Color loop
        Count := Count + Integer (Color'Pos (I)) + 1;
    end loop;
    for I in reverse Upper loop
        Count := Count + 1;
    end loop;
    if Count /= 8 then raise Program_Error; end if;
    W := Word'(High);
    if W /= Word'(High) or Raw (W'Address, 4, 1) /= 2147483647 then
        raise Program_Error;
    end if;
    Rotate (X);
    Swap_Color (X, A (Red));
    if X /= Blue or A (Red) /= Green then raise Program_Error; end if;
    Set_Blue (X);
    begin
        Invalid (X);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if X /= Blue then raise Program_Error; end if;
    begin
        X := Invalid_Result;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if X /= Blue then raise Program_Error; end if;
    Put_Line ("enumeration codes preserve storage and positional operations");
end EnumRepresentation;
