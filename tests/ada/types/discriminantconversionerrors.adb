procedure DiscriminantConversionErrors is
    type Pointer is access all Integer;
    type Rec (D : access Integer) is limited null record;
    type Plain (D : access Integer) is null record;
    Outer : aliased Integer := 1;
    procedure Nested is
        Object : Rec (Outer'Access);
        Alias : Rec renames Object;
        A : Pointer := Pointer (Object.D);
        B : Pointer := Pointer (Alias.D);
        Plain_Object : Plain (Outer'Access);
        C : Pointer := Pointer (Plain_Object.D);
    begin
        null;
    end Nested;
begin
    null;
end DiscriminantConversionErrors;
