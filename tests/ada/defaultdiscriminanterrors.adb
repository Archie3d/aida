procedure DefaultDiscriminantErrors is
    type Partial (A : Integer := 1; B : Integer) is null record;
    type Other_Partial (A : Integer; B : Integer := 1) is null record;
    type Wrong (D : Integer := True) is null record;
    type Indefinite (D : Integer) is null record;
    type Container is record
        Field : Indefinite;
    end record;
    type Items is array (1 .. 2) of Indefinite;
    type Ref is access Indefinite;
    P : Ref := new Indefinite;
    type Defaulted (D : Boolean := False) is null record;
    Boxed : Defaulted (True) := (D => <>);
    Others_Boxed : Defaulted (True) := (others => <>);
    V : Defaulted;
    I : Integer;
    B : Boolean;
begin
    V.D := True;
    B := Defaulted'Constrained;
    B := I'Constrained;
    B := V'Constrained (1);
end DefaultDiscriminantErrors;
