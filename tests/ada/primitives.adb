with Ada.Text_IO; use Ada.Text_IO;
procedure Primitives is
    Captured : Integer := 10;
    package Base is
        type Number is range 0 .. 100;
        subtype Small is Number range 0 .. 5;
        function Make (Value : Integer := 3) return Number;
        function Read (Value : Number) return Integer;
        procedure Bump (Value : in out Number; Amount : Integer := 1);
        procedure Reset (Value : out Number);
        function "+" (Left, Right : Number) return Number;
        function "=" (Left, Right : Number) return Boolean;
        type Early is new Number;
        function Late (Value : Number) return Integer;
        type Box is record
            Value : Integer;
        end record;
        function Make_Box (Value : Integer) return Box;
        procedure Bump (Value : in out Box);
        type List is array (Positive range <>) of Integer;
        function Copy (Value : List) return List;
        type Choice is (First, Second);
        function Read (Value : Choice) return Integer;
    end Base;
    package body Base is
        function Make (Value : Integer := 3) return Number is
        begin
            return Number (Value);
        end Make;
        function Read (Value : Number) return Integer is
        begin
            return Integer (Value) + Captured;
        end Read;
        procedure Bump (Value : in out Number; Amount : Integer := 1) is
        begin
            Value := Number (Integer (Value) + Amount);
        end Bump;
        procedure Reset (Value : out Number) is
        begin
            Value := 4;
        end Reset;
        function "+" (Left, Right : Number) return Number is
        begin
            return Number (Integer (Left) + Integer (Right) + 1);
        end "+";
        function "=" (Left, Right : Number) return Boolean is
        begin
            return Integer (Left) mod 2 = Integer (Right) mod 2;
        end "=";
        function Late (Value : Number) return Integer is
        begin
            return Integer (Value);
        end Late;
        function Make_Box (Value : Integer) return Box is
        begin
            return (Value => Value);
        end Make_Box;
        procedure Bump (Value : in out Box) is
        begin
            Value.Value := Value.Value + 1;
        end Bump;
        function Copy (Value : List) return List is
        begin
            return Value;
        end Copy;
        function Read (Value : Choice) return Integer is
        begin
            return Choice'Pos (Value);
        end Read;
    end Base;
    package Derived is
        type Number is new Base.Small;
        type Box is new Base.Box;
        type List is new Base.List;
        type Choice is new Base.Choice;
        overriding function Read (Value : Number) return Integer;
        type Further is new Number;
    end Derived;
    package body Derived is
        overriding function Read (Value : Number) return Integer is
        begin
            return Integer (Value) + 100;
        end Read;
    end Derived;
    use all type Derived.Number;
    use all type Derived.Box;
    use all type Derived.List;
    use all type Derived.Choice;
    N : Derived.Number := Make;
    B : Derived.Box := Make_Box (7);
    L : Derived.List (2 .. 3) := (11, 12);
    C : Derived.Choice := Second;
    F : Derived.Further := Derived.Make (2);
    Wide : Derived.Number'Base := Make (80);
    function Read_Alias (Value : Derived.Number) return Integer renames Derived.Read;
begin
    Bump (N, Amount => 1);
    Put_Line (Integer'Image (Read_Alias (N)));
    Reset (N);
    Put_Line (Integer'Image (Derived.Read (N + 0)));
    Put_Line (Boolean'Image (N = Make (2)));
    Put_Line (Boolean'Image (N /= Make (2)));
    Bump (B);
    Put_Line (Integer'Image (B.Value));
    L := Copy (L);
    Put_Line (Integer'Image (L (3)));
    Put_Line (Integer'Image (Read (C)));
    Put_Line (Integer'Image (Derived.Read (F)));
    Put_Line (Integer'Image (Late (Wide)));
    declare
        type Local is new Base.Number;
        overriding function Read (Value : Local) return Integer is
        begin
            return Integer (Value) + 200;
        end Read;
        type Leaf is new Local;
        X : Leaf := Make (1);
    begin
        Put_Line (Integer'Image (Read (X)));
        Bump (X);
        Put_Line (Integer'Image (Read (X)));
    end;
    Put_Line (Integer'Image (Base.Read (Base.Early'(2))));
    declare
        type Local_Number is range 0 .. 100;
        overriding function "+" (X, Y : Local_Number) return Local_Number is
        begin
            return Local_Number (Integer (X) + Integer (Y) + 10);
        end "+";
        overriding function "=" (X, Y : Local_Number) return Boolean is
        begin
            return Integer (X) mod 2 = Integer (Y) mod 2;
        end "=";
        type Child_Number is new Local_Number;
        X : Child_Number := 1;
    begin
        Put_Line (Integer'Image (Integer (X + X)));
        Put_Line (Boolean'Image (X /= 3));
    end;
end Primitives;
