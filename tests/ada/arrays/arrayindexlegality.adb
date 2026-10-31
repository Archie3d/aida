with Ada.Text_IO;
with Array_Index_Model; use Array_Index_Model;
procedure ArrayIndexLegality is
    generic
        type Matrix is array (Row, Choice) of Integer;
    procedure Fill_Closed (Value : in out Matrix);
    procedure Fill_Closed (Value : in out Matrix) is
    begin
        Value := (others => (others => 11));
    end Fill_Closed;
    generic
        type Matrix is array (Row range <>, Choice range <>) of Integer;
    procedure Fill_Open (Value : in out Matrix);
    procedure Fill_Open (Value : in out Matrix) is
    begin
        Value := (others => (others => 17));
    end Fill_Open;
    procedure Set_Closed is new Fill_Closed (Closed);
    procedure Set_Open is new Fill_Open (Open);
    C : Closed := (others => (others => 1));
    O : Open (-1 .. 1, First .. Last) := (others => (others => 2));
    E : Explicit_Ranges := (others => (others => 3));
    B : Bare_Subtypes := (others => (others => (others => 4)));
    U : Boxed_Subtypes (-1 .. 1, First .. Last, False .. True) := (others => (others => (others => 5)));
    R : Raw_Ranges := (others => (others => 6));
    M : Mixed_Notations := (others => (others => 7));
    D : Dynamic := (others => (others => 8));
    N : Open (1 .. -1, First .. Last);
    A : array (Row, Choice) of Integer := (others => (others => 9));
    V : array (Row range <>, Choice range <>) of Integer := ((10, 12), (13, 14), (15, 16));
begin
    Limit := 100;
    Set_Closed (C);
    Set_Open (O);
    if C (-1, Last) /= 11 or else O (1, First) /= 17
        or else E (0, First) /= 3 or else B (1, Last, True) /= 4
        or else U (-1, First, False) /= 5 or else R (2, 'B') /= 6
        or else M (0, Last) /= 7 or else D (2, True) /= 8
        or else D'Last (1) /= 2 or else N'Length (1) /= 0
        or else A (1, Last) /= 9 or else V (-1, First) /= 10
        or else V (1, Last) /= 16 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("array index definitions passed");
end ArrayIndexLegality;
