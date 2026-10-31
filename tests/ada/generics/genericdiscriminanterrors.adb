procedure GenericDiscriminantErrors is
    subtype Small is Integer range 1 .. 3;
    type One (A : Integer) is null record;
    type Two (A, B : Integer) is null record;
    type Narrow (A : Small) is null record;
    subtype Fixed is One (1);
    type Plain is null record;
    generic
        type T (D : Integer) is private;
    package G is
    end G;
    package Bad_Count is new G (Two);
    package Bad_Type is new G (Narrow);
    package Bad_Constraint is new G (Fixed);
    package Bad_Plain is new G (Plain);
    generic
        type T (D : Integer) is private;
    procedure Write (X : in out T);
    procedure Write (X : in out T) is
    begin
        X.D := 2;
    end Write;
    generic
        type T (D : Integer) is private;
    package Uninitialized is
        X : T;
    end Uninitialized;
    generic
        type T (D : Float) is private;
    package Bad_Discriminant is
    end Bad_Discriminant;
    package Hidden is
        type Unknown (<>) is private;
        type No_Part is private;
    private
        type Unknown (D : Integer) is null record;
        type No_Part (D : Integer := 1) is null record;
    end Hidden;
    package Bad_Unknown is new G (Hidden.Unknown);
    package Bad_Hidden is new G (Hidden.No_Part);
    type Link (P : access Integer) is limited null record;
    generic
        type T (P : not null access Integer) is limited private;
    package Pointer_View is
    end Pointer_View;
    package Bad_Null is new Pointer_View (Link);
begin
    null;
end GenericDiscriminantErrors;
