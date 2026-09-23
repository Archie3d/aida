with Ada.Text_IO; use Ada.Text_IO;
procedure Generic_Defaults is
    function Original (X : Integer) return Integer is
    begin
        return X + 10;
    end Original;
    generic
        with function Transform (X : Integer) return Integer is Original;
        with function Again (X : Integer) return Integer is Transform;
    function Twice (X : Integer) return Integer;
    function Twice (X : Integer) return Integer is
    begin
        return Again (Transform (X));
    end Twice;

    generic
        type T is range <>;
        with function Combine (X : T; Y : T := 2) return T is "+";
        with function Negative (X : T) return T is "-";
        with function Different (X, Y : T) return Boolean is "/=";
    function Calculate (X : T) return T;
    function Calculate (X : T) return T is
    begin
        if Different (X, 0) then
            return Negative (Combine (X));
        end if;
        return 0;
    end Calculate;
    function Small is new Calculate (Integer);
    function Wide is new Calculate (Long_Integer);

    generic
        with function Original (X : Integer) return Integer is <>;
    function Boxed (X : Integer) return Integer;
    function Boxed (X : Integer) return Integer is
    begin
        return Original (X);
    end Boxed;

    generic
        with function Compare (X, Y : Integer) return Boolean is "<";
    function Earlier return Boolean;
    function Earlier return Boolean is
    begin
        return Compare (1, 2);
    end Earlier;
    function "<" (X, Y : Integer) return Boolean is
    begin
        return X > Y;
    end "<";
    function Before_Operator is new Earlier;

    generic
        with function "=" (X, Y : Integer) return Boolean;
    function Unequal return Boolean;
    function Unequal return Boolean is
    begin
        return 1 /= 2;
    end Unequal;
    function Complement is new Unequal (">");

    subtype Index is Integer range 2 .. 3;
    type Matrix is array (Index, Index) of Integer;
    generic
        type Table is array (Index, Index) of Integer;
    function Sum (Values : Table) return Integer;
    function Sum (Values : Table) return Integer is
    begin
        return Values (2, 2) + Values (2, 3) + Values (3, 2) + Values (3, 3);
    end Sum;
    function Matrix_Sum is new Sum (Matrix);
    Values : Matrix := ((1, 2), (3, 4));
begin
    declare
        function Original (X : Integer) return Integer is
        begin
            return X + 100;
        end Original;
        function Named is new Twice;
        function At_Site is new Boxed;
    begin
        if Named (1) /= 21 or else At_Site (1) /= 101 then
            raise Program_Error;
        end if;
    end;
    if Small (5) /= -7 or else Wide (9) /= -11 or else not Before_Operator
        or else not Complement then
        raise Program_Error;
    end if;
    if Matrix_Sum (Values) /= 10 then
        raise Program_Error;
    end if;
    Put_Line ("generic defaults, forwarding, unary operators, and constrained arrays");
end Generic_Defaults;
