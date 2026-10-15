procedure DefaultBindingErrors is
    A, B : String (1 .. 3);
    procedure Attribute_Default (X : Integer := A'Length);
    procedure Attribute_Default (X : Integer := B'Length) is null;
    package Values is
        subtype Small is Integer range 1 .. 3;
    end Values;
    use Values;
    procedure Membership_Default (X : Boolean := 1 in Small);
    subtype Small is Integer range 1 .. 3;
    procedure Membership_Default (X : Boolean := 1 in Small) is null;
    procedure Structure_Default (X : Integer := 1 + 2);
    procedure Structure_Default (X : Integer := 3) is null;
    type Pair is record
        First, Second : Integer;
    end record;
    procedure Record_Default (X : Pair := (First => 1, Second => 2));
    procedure Record_Default (X : Pair := (First => 1, Second => 3)) is null;
begin
    null;
end DefaultBindingErrors;
