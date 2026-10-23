with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure RuntimeAllocators is
    N : Integer := 3;
    subtype Text is String (2 .. N);
    type Pointer is access String;
    type Fixed_Pointer is access Text;
    procedure Free is new Ada.Unchecked_Deallocation (String, Pointer);
    procedure Free_Fixed is new Ada.Unchecked_Deallocation (Text, Fixed_Pointer);
    P : Pointer := new String (4 .. N + 2);
    Q : Pointer := new String'("abc");
    R : Fixed_Pointer := new Text;
    S : Fixed_Pointer := new Text'("xy");
    Calls : Integer := 0;
    function Bound return Integer is
    begin
        Calls := Calls + 1;
        return Calls + 2;
    end Bound;
    function Make return String is
    begin
        Calls := Calls + 1;
        return (5 .. 7 => 'z');
    end Make;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    type Matrix_Pointer is access Matrix;
    procedure Free_Matrix is new Ada.Unchecked_Deallocation (Matrix, Matrix_Pointer);
    M : Matrix_Pointer := new Matrix (2 .. N, 5 .. N + 3);
begin
    P.all := "pq";
    R.all := "rs";
    if P.all'First /= 4 or P.all'Last /= 5 or P (4) /= 'p'
        or Q.all /= "abc" or R.all'First /= 2 or R (3) /= 's' or S.all /= "xy"
        or M.all'Length (2) /= 2 then raise Program_Error; end if;
    M (2, 5) := 17;
    if M (2, 5) /= 17 or M.all'Size /= 128 then raise Program_Error; end if;
    Free (P);
    P := new String'(Make);
    if Calls /= 1 or P.all'First /= 5 or P.all /= "zzz" then raise Program_Error; end if;
    for J in 1 .. 3 loop
        declare
            Temp : Pointer := new String (2 .. Bound);
        begin
            if Temp.all'Length /= Calls + 1 then raise Program_Error; end if;
            Free (Temp);
        end;
    end loop;
    if P.all'First /= 5 or P.all'Length /= 3 then raise Program_Error; end if;
    begin
        R := new Text'("bad");
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Free (P); Free (Q); Free_Fixed (R); Free_Fixed (S); Free_Matrix (M);
    if P /= null or R /= null then raise Program_Error; end if;
    declare
        type Slide_Pointer is access String (2 .. N);
        Slid : Slide_Pointer := new String'(11 .. N + 9 => 's');
    begin
        if Slid.all'First /= 2 or Slid.all'Last /= 3 or Slid.all /= "ss" then raise Program_Error; end if;
    end;
    Ada.Text_IO.Put_Line ("runtime allocators ok");
end RuntimeAllocators;
