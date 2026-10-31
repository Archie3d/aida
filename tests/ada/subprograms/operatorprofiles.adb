with Ada.Text_IO; use Ada.Text_IO;
procedure OperatorProfiles is
    type Box is record
        Value : Integer;
    end record;
    function "+" (Value : Box) return Integer is
        function Get return Integer is
        begin
            return Value.Value;
        end Get;
    begin
        return Get;
    end "+";
    function "/" (Left, Right : Box) return Integer is
    begin
        return Left.Value / Right.Value;
    end "/";
    function "mod" (Left, Right : Box) return Integer is
    begin
        return Left.Value mod Right.Value;
    end "mod";
    function "rem" (Left, Right : Box) return Integer is
    begin
        return Left.Value rem Right.Value;
    end "rem";
    function "**" (Left, Right : Box) return Integer is
    begin
        return Left.Value ** Right.Value;
    end "**";
    function "-" (Left, Right : Box) return Box is
    begin
        return (Value => Left.Value - Right.Value);
    end "-";
    function "<=" (Left, Right : Box) return Boolean is
    begin
        return Left.Value <= Right.Value;
    end "<=";
    function ">" (Left, Right : Box) return Boolean is
    begin
        return Left.Value > Right.Value;
    end ">";
    function ">=" (Left, Right : Box) return Boolean is
    begin
        return Left.Value >= Right.Value;
    end ">=";
    -- Non-Boolean comparison results are legal and selected by context.
    function "=" (Left, Right : Box) return Integer is
    begin
        return Left.Value - Right.Value;
    end "=";
    function "/=" (Left, Right : Box) return Integer is
    begin
        return Left.Value + Right.Value;
    end "/=";
    function "&" (Left : String; Right : Character) return String is
    begin
        return Left;
    end "&";
    A : Box := (Value => 7);
    B : Box := (Value => 2);
    C : Box;
    I : Integer;
    Calls : Integer := 0;
    package More is
        function "+" (Left, Right : Box) return Box;
    end More;
    package body More is
        function "+" (Left, Right : Box) return Box is
        begin
            Calls := Calls + 1;
            return (Value => Left.Value + Right.Value);
        end "+";
    end More;
    use More;
    function Take return Box is
    begin
        Calls := Calls + 1;
        return A;
    end Take;
begin
    Put_Line (Integer'Image (+A));
    Put_Line (Integer'Image (A / B));
    Put_Line (Integer'Image (A mod B));
    Put_Line (Integer'Image (A rem B));
    Put_Line (Integer'Image (B ** B));
    C := A - B;
    Put_Line (Integer'Image (C.Value));
    if B <= A and A > B and A >= A then
        Put_Line ("relations");
    end if;
    I := A = B;
    Put_Line (Integer'Image (I));
    I := A /= B;
    Put_Line (Integer'Image (I));
    if A /= B then
        Put_Line ("predefined equality");
    end if;
    Put_Line (("x" & '!') & "y");
    -- A directly visible unary + must not hide a use-visible binary +.
    C := Take + Take;
    Put_Line (Integer'Image (C.Value));
    Put_Line (Integer'Image (Calls));
    declare
        function "+" (Left, Right : Box) return Box is
        begin
            return (Value => 42);
        end "+";
    begin
        C := A + B;
        Put_Line (Integer'Image (C.Value));
        C := More."+" (A, B);
        Put_Line (Integer'Image (C.Value));
    end;
    declare
        function "-" (Value : Box) return Box is
        begin
            raise Constraint_Error;
            return Value;
        end "-";
    begin
        C := -A;
    exception
        when Constraint_Error => Put_Line ("operator exception");
    end;
end OperatorProfiles;
