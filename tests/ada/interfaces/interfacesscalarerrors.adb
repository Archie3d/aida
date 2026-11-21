with Interfaces;
with Interfaces.C;

procedure InterfacesScalarErrors is
    package I renames Interfaces;
    package C renames Interfaces.C;
    Integer_Value : I.Integer_32 := 1;
    Char_Value : Character := 'A';
    Float_Value : I.IEEE_Float_32 := 1.0;
    Bad_Int : C.int := Integer_Value;
    Bad_Char : C.char := Char_Value;
    Bad_Float : C.C_float := Float_Value;
    type Shift_Pointer is access function (Value : I.Unsigned_8; Amount : Natural) return I.Unsigned_8;
    Bad_Access : Shift_Pointer := I.Shift_Left'Access;
    procedure Ordinary;
    pragma Import (Intrinsic, Ordinary);
    procedure Ordinary is
    begin
        null;
    end Ordinary;
begin
    null;
end InterfacesScalarErrors;
