-- Legal counterparts for the selected B-test diagnostic families.
procedure Legal is
    type Parent is range 0 .. 10;
    type Child is new Parent;
    type Enum is (One, Two);
    type Derived_Enum is new Enum;
    type Vector is array (Integer range <>) of Integer;
    type Matrix is array (Integer range <>, Boolean range <>) of Integer;
    type Fixed_Matrix is array (1 .. 2, False .. True) of Integer;
    type Rec (D : Boolean := False) is record
        Value : Integer;
    end record;
    type Ptr is access all Integer;
    A : aliased Integer := 1;
    P : Ptr := A'Access;
    R : Rec := (False, 1);
    V : Vector := (1, 2);
    C : Child := Child (Parent'(2));
    E : Derived_Enum := One;
    package Names is
        Item : Integer := 1;
        subtype Other is Integer;
    end Names;
    procedure Call (X : Integer; Y : in out Integer; Z : Integer := 0) is
    begin
        Y := X + Z;
    end Call;
    function Value (X : Integer) return Integer is
    begin
        return X;
    end Value;
    generic
        with procedure Action (X : Integer) is null;
    package Gen is
    end Gen;
    package Instance is new Gen;
begin
    Call (1, A);
    Call (X => Value (1), Z => 2, Y => A);
    C := C + 1;
    E := Derived_Enum'Succ (E);
    R.Value := P.all;
    V := (others => 3);
end Legal;
