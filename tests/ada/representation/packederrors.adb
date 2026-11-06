with System;
procedure PackedErrors is
    type Numbers is array (1 .. 3) of Integer;
    pragma Pack (Numbers);
    type Matrix is array (1 .. 2, 1 .. 2) of Boolean;
    pragma Pack (Matrix);
    type Aliased_Bits is array (1 .. 3) of aliased Boolean;
    pragma Pack (Aliased_Bits);
    type Bits is array (1 .. 9) of Boolean;
    pragma Pack (Bits);
    pragma Pack (Bits);
    subtype View is Bits;
    pragma Pack (View);
    A : Bits := (others => False);
    B : Bits := (others => True);
    S : Bits := A (1 .. 9);
    C : Bits := A & B;
    Address : System.Address := A (1)'Address;
    type Late is array (1 .. 3) of Boolean;
    Frozen : Late := (others => False);
    pragma Pack (Late);
begin
    null;
end PackedErrors;
