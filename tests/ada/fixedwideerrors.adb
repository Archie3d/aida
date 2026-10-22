procedure FixedWideErrors is
    Dynamic : Long_Float := 0.125;
    type A is delta 0.25 range -10.0 .. 10.0;
    for A'Small use 0.0;
    type B is delta 0.25 range -10.0 .. 10.0;
    for B'Small use 0.5;
    type C is delta 0.25 range -10.0 .. 10.0;
    for C'Small use 0.1;
    type D is delta 0.25 range -10.0 .. 10.0;
    for D'Small use Dynamic;
    type E is delta 0.25 range -10.0 .. 10.0;
    for E'Small use 0.125;
    for E'Small use 0.125;
    type F is new E;
    for F'Small use 0.125;
    subtype G is E range 0.0 .. 1.0;
    for G'Small use 0.125;
    for Integer'Small use 0.125;
    type H is delta 0.25 range -10.0 .. 10.0;
    X : H := 1.0;
    for H'Small use 0.125;
    Bad_Radix : Integer := H'Machine_Radix (1);
    Bad_Image : String := H'Image (Dynamic);
    Bad_Round : Integer := Integer'Machine_Rounds;
    package Hidden is
        type T is private;
    private
        type T is delta 0.25 range -10.0 .. 10.0;
    end Hidden;
    Hidden_Small : Long_Float := Hidden.T'Small;
    type Self_Small is delta 2.0 range -10.0 .. 10.0;
    for Self_Small'Small use Self_Small'Small;
begin
    null;
end FixedWideErrors;
