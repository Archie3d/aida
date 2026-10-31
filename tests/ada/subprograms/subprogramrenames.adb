with Ada.Text_IO;
procedure Subprogramrenames is
    procedure Print (Message : String) renames Ada.Text_IO.Put_Line;
    Offset : Integer := 10;
    subtype Small is Integer range 0 .. 20;
    function Add (X : Small := 1) return Integer is
    begin
        return X + Offset;
    end Add;
    function Add (X : Float) return Float is
    begin
        return X + 0.5;
    end Add;
    function Alias (Value : Integer := 3) return Integer renames Add;
    function Again (Argument : Integer := 4) return Integer renames Alias;
    function Real_Add (Value : Float) return Float renames Add;
    procedure Bump (X : in out Integer) is
    begin
        X := X + 1;
    end Bump;
    procedure Increment (Value : in out Integer) renames Bump;
    function Completed (X : Small := 5) return Integer;
    function Completed (X : Small) return Integer renames Add;
    function Sum (Left, Right : Small) return Small renames "+";
    type Color is (Red, Blue);
    function Favorite return Color renames Blue;
    type Callback is access function (X : Small) return Integer;
    F : Callback := Alias'Access;
    X : Integer := 7;
    generic
        type Number is range <>;
        with function Original (X : Number) return Number;
    procedure Generic_Test;
    procedure Generic_Test is
        function Copy (Value : Number) return Number renames Original;
    begin
        if Copy (1) > 0 then
            Print ("generic rename");
        end if;
    end Generic_Test;
    procedure Check is new Generic_Test (Integer, Add);
begin
    Print (Integer'Image (Alias));
    Print (Integer'Image (Again (Argument => 2)));
    Print (Integer'Image (Again));
    Print (Integer'Image (Completed));
    Increment (Value => X);
    Print (Integer'Image (X));
    Print (Integer'Image (Sum (20, 20)));
    if Favorite = Blue and Real_Add (1.0) = 1.5 then
        Print ("overloads and literal");
    end if;
    if F = Add'Access then
        Print ("same callback");
    end if;
    begin
        X := Alias (30);
    exception
        when Constraint_Error => Print ("target constraint");
    end;
    Check;
end Subprogramrenames;
