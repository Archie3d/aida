with Ada.Text_IO; use Ada.Text_IO;
procedure AttributeBounds is
    Calls : Integer := 0;
    type Vector is array (Long_Integer range <>) of Integer;
    type Matrix is array (Integer range <>, Long_Integer range <>) of Integer;
    V : Vector (2 .. 4) := (1, 2, 3);
    M : Matrix (1 .. 2, 3 .. 5) := (others => (others => 0));
    subtype Pair is Vector (6 .. 7);
    type Link is access Pair;
    P : Link := new Pair'(11, 12);
    Bound : Long_Integer;
    function Get return Link is
    begin
        Calls := Calls + 1;
        return P;
    end Get;
    function Make return Vector is
    begin
        Calls := Calls + 1;
        return V;
    end Make;
    function Fixed_Make return Pair is
    begin
        Calls := Calls + 1;
        return (11, 12);
    end Fixed_Make;
    procedure Show (A : Vector; B : Matrix) is
        Dimension : constant := 1 + 1;
    begin
        Put_Line (Long_Integer'Image (A'First + 4294967296));
        Put_Line (Long_Integer'Image (A'Last));
        Put_Line (Long_Integer'Image (B'First (Dimension) + 4294967296));
        Put_Line (Long_Integer'Image (B'Last (2)));
        for J in B'Range (2) loop
            Put_Line (Long_Integer'Image (J));
        end loop;
    end Show;
    subtype Empty is Integer range 3 .. 2;
    Empty_Array : Vector (5 .. 4);
begin
    Show (V, M);
    Put_Line (Integer'Image (Empty'First));
    Put_Line (Integer'Image (Empty'Last));
    for I in Empty'Range loop
        Put_Line ("MISSED null scalar range");
    end loop;
    for I in Empty_Array'Range loop
        Put_Line ("MISSED null array range");
    end loop;
    for I in Make'Range loop
        null;
    end loop;
    Put_Line (Integer'Image (Calls));
    Put_Line (Long_Integer'Image (Fixed_Make'First));
    Put_Line (Integer'Image (Calls));
    for I in Fixed_Make'Range loop
        null;
    end loop;
    Put_Line (Integer'Image (Calls));
    for I in Get'Range loop
        null;
    end loop;
    Put_Line (Integer'Image (Calls));
    P := null;
    begin
        Bound := P'First;
        Put_Line ("MISSED null access prefix");
    exception
        when Constraint_Error => Put_Line ("null access prefix");
    end;
end AttributeBounds;
