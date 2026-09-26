procedure Fixederrors is
    type Bad_Delta is delta 0.0 range -1.0 .. 1.0;
    type Too_Fine is delta 0.000000000001 range -1.0 .. 1.0;
    type Fixed is delta 0.125 range -10.0 .. 10.0;
    type Other is delta 0.125 range -10.0 .. 10.0;
    subtype Bad_Bounds is Fixed range True .. False;
    for Fixed'Size use 32;
    A : Fixed := 1.0;
    B : Other := 2.0;
    C : Fixed := A + B;
    D : Fixed := A mod A;
    E : Fixed := A ** 2;
    F : Fixed := True;
begin
    null;
end Fixederrors;
