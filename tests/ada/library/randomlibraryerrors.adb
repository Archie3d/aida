with Ada.Numerics.Float_Random;
with Ada.Numerics.Discrete_Random;
procedure RandomLibraryErrors is
    package FR renames Ada.Numerics.Float_Random;
    package One is new Ada.Numerics.Discrete_Random (Integer);
    package Two is new Ada.Numerics.Discrete_Random (Integer);
    package Bad is new Ada.Numerics.Discrete_Random (Float);
    A, B : FR.Generator;
    S : FR.State;
    X : One.State;
    Y : Two.State;
    G : One.Generator;
    V : Float;
    function Wrong (Gen : in out FR.Generator) return FR.Uniformly_Distributed renames FR.Random;
    procedure Bad_Reset (Gen : in out FR.Generator; Initiator : Integer) renames FR.Reset;
begin
    A := B;
    if A = B then null; end if;
    X := Y;
    S := X;
    FR.Reset (A, X);
    One.Reset (G, S);
    V := FR.Random (G);
    X := One.Value (123);
end RandomLibraryErrors;
