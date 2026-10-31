with Ada.Finalization;
procedure ConcatErrors is
    type Vector is array (Positive range <>) of Integer;
    type Other is array (Positive range <>) of Integer;
    A : Vector (1 .. 1) := (others => 1);
    B : Other (1 .. 1) := (others => 1);
    Mixed : Vector := A & B;
    type Matrix is array (1 .. 1, 1 .. 1) of Integer;
    M : Matrix := (others => (others => 1));
    Multi : Matrix := M & M;
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
    type Guards is array (Positive range <>) of Guard;
    G : Guards (1 .. 1);
    Limited_Result : Guards := G & G;
    procedure Take (X : Vector) is
    begin
        null;
    end Take;
    procedure Take (X : Other) is
    begin
        null;
    end Take;
begin
    Take (1 & 2);
end ConcatErrors;
