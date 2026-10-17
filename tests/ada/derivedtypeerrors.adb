procedure DerivedTypeErrors is
    type Incomplete;
    type Too_Early is new Incomplete;
    type Incomplete is null record;
    type Parent (N : Integer) is null record;
    type Replaced (M : Integer) is new Parent;
    subtype Fixed is Parent (1);
    type Child is new Fixed;
    subtype Changed is Child (2);
    package Hidden is
        type Secret is private;
    private
        type Secret is new Integer range 0 .. 10;
    end Hidden;
    package Deferred is
        type Secret is private;
        type Too_Early is new Secret;
    private
        type Secret is null record;
    end Deferred;
    type Hidden_Child is new Hidden.Secret;
    Secret_Value : Hidden.Secret;
    Exposed : Integer := Integer (Secret_Value);
    type Color is (Red, Blue);
    type Shade is new Color;
    type Unrelated is (Red, Blue);
    C : Color := Red;
    S : Shade := C;
    U : Unrelated := Unrelated (C);
    type Box is record
        X : Integer;
    end record;
    type Other_Box is record
        X : Integer;
    end record;
    B : Box := (X => 1);
    O : Other_Box := Other_Box (B);
    type Link is access Integer;
    type Other_Link is access Integer;
    L : Link := null;
    M : Other_Link := Other_Link (L);
    type Box_Child is new Box;
    D : Box_Child := Box_Child ((X => 1));
    procedure Set (X : out Shade) is
    begin
        X := Red;
    end Set;
begin
    Shade (C) := Red;
    Set (Shade (C));
    Box_Child (B).X := 2;
end DerivedTypeErrors;
