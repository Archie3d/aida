package Scalar_Dynamic_Model is
    Limit : Integer := 12;
    Real_Limit : Long_Float := 4.5;
    subtype Small is Integer range 2 .. Limit;
    subtype Real_Range is Long_Float range -Real_Limit .. Real_Limit;
    type Fraction is delta 0.25 range -64.0 .. 64.0;
    Fixed_Limit : Fraction := 2.0;
    subtype Fixed_Range is Fraction range -Fixed_Limit .. Fixed_Limit;
    type Holder is record
        Value : Integer range 2 .. Limit := 3;
        Real_Value : Long_Float range -Real_Limit .. Real_Limit := 1.5;
    end record;
    type Vector is array (1 .. 2) of Integer range 2 .. Limit;
    generic
        Cap : Integer;
    package Window is
        subtype Bound is Integer range 1 .. Cap;
    end Window;
    package Captured is new Window (Limit);
    function Echo (Value : Small) return Small;
    function Real_Echo (Value : Real_Range) return Real_Range;
    Deferred : constant Small;
private
    Deferred : constant Small := 3;
end Scalar_Dynamic_Model;
