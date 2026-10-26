with Ada.Text_IO;
procedure GenericDerived is
    package Model is
        type Real_Number is digits 6 range -20.0 .. 20.0;
        type Real_Child is new Real_Number;
        type Fixed_Number is delta 0.25 range 0.0 .. 10.0;
        type Fixed_Child is new Fixed_Number;
        type Modular_Number is mod 256;
        type Modular_Child is new Modular_Number;
        type Number is range -100 .. 100;
        function Read (X : Number) return Integer;
        type Child is new Number;
        overriding function Read (X : Child) return Integer;
        type Color is (Red, Green, Blue);
        type Color_Child is new Color;
        type Vector is array (Integer range <>) of Integer;
        type Vector_Child is new Vector;
        type Box is record
            Value : Integer;
        end record;
        type Box_Child is new Box;
        type Pointer is access Integer;
        type Pointer_Child is new Pointer;
    end Model;
    package body Model is
        function Read (X : Number) return Integer is
        begin
            return Integer (X) + 1;
        end Read;
        function Read (X : Child) return Integer is
        begin
            return Integer (X) + 100;
        end Read;
    end Model;
    generic
        type T is new Model.Number;
    function Read_Ancestor (X : T) return Integer;
    function Read_Ancestor (X : T) return Integer is
    begin
        return Read (X);
    end Read_Ancestor;
    function Read_Child is new Read_Ancestor (Model.Child);
    subtype Small is Model.Child range 1 .. 5;
    function Read_Small is new Read_Ancestor (Small);
    generic
        type T is new Model.Color;
    function Last_Color return T;
    function Last_Color return T is
    begin
        return Blue;
    end Last_Color;
    function Last_Child is new Last_Color (Model.Color_Child);
    generic
        type T is new Model.Vector;
    function Copy (X : T) return T;
    function Copy (X : T) return T is
    begin
        return X;
    end Copy;
    function Copy_Child is new Copy (Model.Vector_Child);
    generic
        type T is new Model.Box;
    procedure Change (X : in out T);
    procedure Change (X : in out T) is
    begin
        X.Value := X.Value + 1;
    end Change;
    procedure Change_Child is new Change (Model.Box_Child);
    generic
        type T is new Model.Pointer;
    procedure Change_Target (X : T);
    procedure Change_Target (X : T) is
    begin
        X.all := X.all + 1;
    end Change_Target;
    procedure Update is new Change_Target (Model.Pointer_Child);
    generic
        type T is new Model.Real_Number;
    function Double (X : T) return T;
    function Double (X : T) return T is
    begin
        return X * 2.0;
    end Double;
    function Double_Real is new Double (Model.Real_Child);
    generic
        type T is new Model.Fixed_Number;
    function Step (X : T) return T;
    function Step (X : T) return T is
    begin
        return X + 1.0;
    end Step;
    function Step_Fixed is new Step (Model.Fixed_Child);
    generic
        type T is new Model.Modular_Number;
    function Next_Modular (X : T) return T;
    function Next_Modular (X : T) return T is
    begin
        return X + 1;
    end Next_Modular;
    function Step_Modular is new Next_Modular (Model.Modular_Child);
    use type Model.Real_Child;
    use type Model.Fixed_Child;
    use type Model.Modular_Child;
    use type Model.Color_Child;
    use type Model.Vector_Child;
    procedure Check is
        X : Model.Vector_Child (3 .. 4) := (7, 8);
        Y : Model.Box_Child := (Value => 3);
        P : Model.Pointer_Child := new Integer'(10);
    begin
        if Double_Real (5.0) /= 10.0 or Step_Fixed (1.0) /= 2.0 or Step_Modular (255) /= 0 then
            raise Program_Error;
        end if;
        if Read_Child (4) /= 5 or Read_Small (2) /= 3
            or Last_Child /= Model.Blue or Copy_Child (X) /= X then
            raise Program_Error;
        end if;
        Change_Child (Y);
        Update (P);
        if Y.Value /= 4 or P.all /= 11 then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("generic derived types passed");
end GenericDerived;
