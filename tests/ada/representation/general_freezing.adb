package body General_Freezing is
    function Object_Default (Value : Integer) return Integer is
    begin
        return Value;
    end Object_Default;
    function Make return Source is
    begin
        return (10, 20);
    end Make;
    function Score (Item : Source) return Integer is
    begin
        return Item.Left + Item.Right;
    end Score;
    function Selection (Value : Integer) return Integer is
    begin
        return Value;
    end Selection;
    function Allocation (Value : Pointer) return Integer is
    begin
        return Value.Left + Value.Right;
    end Allocation;
    package Default_Actual is new Defaults;

    procedure Check is
        R : Ranged_Real := 0.5;
        B : Byte_View := -63;
        C : Color_View := Blue;
        V : Vector_Pointer := new Vector'((11, 22));
        H : Holder;
        O : aliased Owner;
    begin
        if R /= 0.5 or else R'Size /= 32 or else B /= -63 or else B'Size /= 8 or else Byte'Alignment /= 1
            or else C /= Blue or else C'Size /= 16
            or else V (1) /= 11 or else V (2) /= 22 or else V.all'Size /= 128
            or else Selection /= 20 or else Allocation /= 30 or else H.Value /= 20
            or else Explicit_Actual.Result /= 9 or else Default_Actual.Result /= 30
            or else Object_Default /= 7 then
            raise Program_Error;
        end if;
        if O.Child.Ref /= O'Access then
            raise Program_Error;
        end if;
    end Check;
end General_Freezing;
