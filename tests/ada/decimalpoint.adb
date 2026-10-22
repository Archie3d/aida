with Ada.Text_IO; use Ada.Text_IO;
with Ada.Text_IO.Decimal_IO;
procedure DecimalPoint is
    type Money is delta 0.01 digits 12;
    type Fine is delta 0.0001 digits 18;
    type Coarse is delta 100.0 digits 8;
    type Tiny is delta 1.0E-18 digits 18;
    type Huge is delta 1.0E18 digits 18;
    type Binary is delta 0.125 range -100.0 .. 100.0;
    type Derived is new Money;
    subtype Small is Money digits 3;
    subtype Restricted is Money digits 4 range -12.0 .. 12.0;
    package IO is new Ada.Text_IO.Decimal_IO (Money);
    generic
        type Num is delta <> digits <>;
    package Operations is
        function Interest (Value, Rate : Num) return Num;
        function Rounded (Value, Rate : Num) return Num;
        Scale : constant Integer := Num'Scale;
        Digits_Count : constant Integer := Num'Digits;
    end Operations;
    package body Operations is
        function Interest (Value, Rate : Num) return Num is
        begin
            return Value * Rate;
        end Interest;
        function Rounded (Value, Rate : Num) return Num is
        begin
            return Num'Round (Value * Rate);
        end Rounded;
    end Operations;
    package Op is new Operations (Money);
    A : Money := 1.25;
    B : Money := 0.50;
    C : Money;
    X : Fine := 1.2350;
    F : Long_Float := 1.235;
    Q : Binary := 1.25;
    Upper : Money := 2.0;
    subtype Window is Money digits 3 range 1.0 .. Upper;
    W : Window := 1.25;
    package W_IO is new Ada.Text_IO.Decimal_IO (Window);
    Text : String (1 .. 10);
    Last : Positive;
    File : File_Type;
    Exact_Product : constant Money := Money'(1.25) * Money'(0.50);
    Exact_Rounded : constant Money := Money'Round (Money'(1.25) * Money'(0.50));
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
begin
    Check (Money'Round (1) = 1.0 and Money'Round (1.235) = 1.24);
    Check (Money'Small = 0.01 and Money'Delta = Money'Small);
    Check (Money'Digits = 12 and Money'Base'Digits = 18 and Money'Scale = 2);
    Check (Small'Digits = 3 and Small'First = -9.99 and Small'Last = 9.99);
    Check (Restricted'Digits = 4 and Restricted'Last = 12.0);
    Check (Coarse'Scale = -2 and Coarse'Aft = 1 and Tiny'Scale = 18);
    Check (Money'Machine_Radix = 10 and Money'Machine_Overflows);
    Check (Money'Fore = 11 and Money'Aft = 2 and Window'Fore = 2);
    Check (Op.Scale = 2 and Op.Digits_Count = 12);
    C := A * B;
    Check (C = 0.62 and Exact_Product = 0.62 and Exact_Rounded = 0.63);
    Check (Money'Round (Standard."*" (A, B)) = 0.63);
    Check (Op.Interest (A, B) = 0.62 and Op.Rounded (A, B) = 0.63);
    C := -A * B;
    Check (C = -0.62 and Money'Round (-A * B) = -0.63);
    C := A / 2;
    Check (C = 0.62 and Money'Round (A / 2) = 0.63);
    C := A / Money'(2.0);
    Check (C = 0.62 and Money'Round (A / Money'(2.0)) = 0.63);
    Check (Money (X) = 1.24 and Money'Round (X) = 1.24);
    Check (Money (F) = 1.24 and Money'Round (F) = 1.24);
    Check (Money (Q) = A and Binary (A) = Q and Long_Float (A) = 1.25);
    Check (Integer (A) = 1 and Integer (Money'(-1.50)) = -2);
    Check (Coarse (150.0) = 200.0 and Coarse (-150.0) = -200.0);
    Check (Tiny'Last = 0.999999999999999999 and Huge'Last = 999999999999999999000000000000000000.0);
    Check (Derived'Scale = 2 and Derived'Digits = 12 and Derived (A) = 1.25);
    Check (Money'Succ (A) = 1.26 and Money'Pred (A) = 1.24);
    Check (Money'Min (A, B) = B and Money'Max (A, B) = A);
    Check (Money'Image (A) = " 1.25" and Money'Value ("1.235") = 1.24);
    IO.Put (Text, A);
    Check (Text = "      1.25");
    IO.Get ("-1.235!", C, Last);
    Check (C = -1.24 and Last = 6);
    W_IO.Put (Text, W);
    Check (Text = "      1.25");
    W_IO.Get ("1.505", W, Last);
    Check (W = 1.51 and Last = 5);
    Create (File, Out_File, "decimaltext.tmp");
    IO.Put (File, A, Fore => 0);
    Put (File, ", -1.235 2.500");
    Reset (File, In_File);
    IO.Get (File, C);
    Check (C = A);
    declare
        Separator : Character;
    begin
        Get (File, Separator);
        Check (Separator = ',');
    end;
    IO.Get (File, C, Width => 7);
    Check (C = -1.24);
    Set_Input (File);
    IO.Get (C);
    Check (C = 2.5);
    Set_Input (Standard_Input);
    Delete (File);
    IO.Put (Text, A, Aft => 1, Exp => 3);
    Check (Text = "   1.3E+00");
    Put_Line ("decimal scales, digits, rounding, arithmetic, conversions and generic I/O");
end DecimalPoint;
