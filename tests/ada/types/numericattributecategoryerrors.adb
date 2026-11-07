procedure NumericAttributeCategoryErrors is
    type Number is range 0 .. 10;
    A : Integer := Number'Aft;
    B : Integer := Number'Fore;
    C : Float := Number'Small;
    D : Float := Number'Delta;
    E : Integer := Number'Emax;
    F : Float := Number'Epsilon;
    G : Float := Number'Large;
    H : Integer := Number'Mantissa;
    I : Integer := Number'Safe_Emax;
    J : Float := Number'Safe_Large;
    K : Float := Number'Safe_Small;
begin
    null;
end NumericAttributeCategoryErrors;
