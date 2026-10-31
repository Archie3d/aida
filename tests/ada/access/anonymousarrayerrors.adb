procedure AnonymousArrayErrors is
    A, B : array (1 .. 2) of Integer := (others => 0);
    Missing : array (Positive range <>) of Integer;
begin
    A := B;
end AnonymousArrayErrors;
