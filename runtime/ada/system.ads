-- The supported QBE targets share a 64-bit, little-endian, byte-addressed model.
package System is
    pragma Pure;

    -- Completed by the compiler's canonical machine-address type. It has no
    -- designated object, access collection or ownership. The existing null
    -- literal and related-address conversion extensions remain available.
    type Address is private;
    Null_Address : constant Address := null;

    type Name is (AIDA_64);
    System_Name : constant Name := AIDA_64;

    Min_Int : constant := Long_Integer'First;
    Max_Int : constant := Long_Integer'Last;
    Storage_Unit : constant := 8;
    Word_Size : constant := Address'Size;
    Memory_Size : constant := 2 ** Word_Size;
    Max_Binary_Modulus : constant := Memory_Size;
    Max_Nonbinary_Modulus : constant := Max_Binary_Modulus - 1;
    Max_Base_Digits : constant := Long_Float'Digits;
    Max_Digits : constant := Max_Base_Digits;
    Max_Mantissa : constant := Long_Integer'Size - 1;
    Fine_Delta : constant := 2.0 ** (2 - Long_Integer'Size);
    Tick : constant := 0.000_001;

    -- The canonical address type supplies these intrinsic operations. Each
    -- renaming gives its ordinary public Ada profile an analyzed declaration.
    function "<" (Left, Right : Address) return Boolean renames "<";
    function "<=" (Left, Right : Address) return Boolean renames "<=";
    function ">" (Left, Right : Address) return Boolean renames ">";
    function ">=" (Left, Right : Address) return Boolean renames ">=";
    function "=" (Left, Right : Address) return Boolean renames "=";
    -- "/=" is implicitly declared with "=".

    type Bit_Order is (High_Order_First, Low_Order_First);
    Default_Bit_Order : constant Bit_Order := Low_Order_First;

    subtype Any_Priority is Integer range 0 .. 31;
    subtype Priority is Any_Priority range Any_Priority'First .. 30;
    subtype Interrupt_Priority is Any_Priority range Priority'Last + 1 .. Any_Priority'Last;
    Default_Priority : constant Priority := (Priority'First + Priority'Last) / 2;
end System;
