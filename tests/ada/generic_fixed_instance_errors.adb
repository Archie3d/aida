procedure Generic_Fixed_Instance_Errors is
    generic
        type Item is delta <>;
    package P is
        Value : Item := Item (2.0);
    end P;
    type Narrow is delta 0.125 range -1.0 .. 1.0;
    package Bad is new P (Narrow);

    generic
        type Item is digits <>;
    package Floating is
    end Floating;
    package Bad_Float is new Floating (Narrow);
begin
    null;
end Generic_Fixed_Instance_Errors;
