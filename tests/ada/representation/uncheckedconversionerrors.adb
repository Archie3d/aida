with Ada.Unchecked_Conversion;
procedure UncheckedConversionErrors is
    type Record_Type is record
        Value : Integer;
    end record;
    function Invalid is new Ada.Unchecked_Conversion (Record_Type, Integer);
    type Bits is array (Integer range <>) of Boolean;
    pragma Pack (Bits);
    function Missing_Bounds is new Ada.Unchecked_Conversion (Integer, Bits);
    type Plain is array (1 .. 4) of Boolean;
    function Not_Packed is new Ada.Unchecked_Conversion (Plain, Integer);
    function Convert is new Ada.Unchecked_Conversion (Integer, Integer);
    type Callback is access function (S : Integer) return Integer;
    Pointer : Callback := Convert'Access;
begin
    null;
end UncheckedConversionErrors;
