with Ada.Text_IO; use Ada.Text_IO;
procedure Callbackchecks is
    subtype First_Range is Integer range 0 .. 10;
    subtype Second_Range is Integer range 0 .. 10;
    type Constrained_Callback is access function (X : First_Range) return First_Range;
    function Identity (Value : Second_Range) return Second_Range is
    begin
        return Value;
    end Identity;
    type Float_Callback is access function (X : Float) return Float;
    function Twice (Value : Float) return Float is
    begin
        return 2.0 * Value;
    end Twice;
    type Pair is record
        X : Integer;
        Y : Integer;
    end record;
    type Pair_Callback is access function return Pair;
    function Get_Pair return Pair is
    begin
        return (1, 2);
    end Get_Pair;
    type Action is access procedure (X : in out Integer);
    procedure Fails (Value : in out Integer) is
    begin
        Value := 99;
        raise Constraint_Error;
    end Fails;
    type Setter is access procedure (X : out Integer);
    procedure Set_Value (Value : out Integer) is
    begin
        Value := 99;
    end Set_Value;
    C : Constrained_Callback := Identity'Access;
    F : Float_Callback := Twice'Access;
    G : Pair_Callback := Get_Pair'Access;
    P : Action := Fails'Access;
    S : Setter := Set_Value'Access;
    generic
        type Number is range <>;
    procedure Generic_Check;
    procedure Generic_Check is
        type Callback is access function (X : Number) return Number;
        function Identity (X : Number) return Number is
        begin
            return X;
        end Identity;
        F : Callback := Identity'Access;
    begin
        if F (1) = 1 then
            Put_Line ("generic callback");
        end if;
    end Generic_Check;
    procedure Check_Integer is new Generic_Check (Integer);
    Value : Integer := 5;
    Small : First_Range := 2;
begin
    Check_Integer;
    Put_Line (Integer'Image (C (X => 7)));
    if F (1.5) = 3.0 then
        Put_Line ("float callback");
    end if;
    Put_Line (Integer'Image (G.all.Y));
    begin
        P (X => Value);
    exception
        when Constraint_Error => Put_Line (Integer'Image (Value));
    end;
    begin
        S (Small);
    exception
        when Constraint_Error => Put_Line (Integer'Image (Small));
    end;
end Callbackchecks;
