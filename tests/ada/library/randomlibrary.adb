with Ada.Text_IO; use Ada.Text_IO;
with Ada.Numerics.Float_Random;
with Ada.Numerics.Discrete_Random;
with Ada.Unchecked_Deallocation;
procedure RandomLibrary is
    package FR renames Ada.Numerics.Float_Random;
    use type FR.State;
    subtype Die is Integer range 1 .. 6;
    package Dice is new Ada.Numerics.Discrete_Random (Die);
    package Other_Dice is new Ada.Numerics.Discrete_Random (Die);
    package Signed is new Ada.Numerics.Discrete_Random (Long_Integer);
    type Word is mod 2 ** 64;
    package Unsigned is new Ada.Numerics.Discrete_Random (Word);
    type Color is (Red, Green, Blue);
    for Color use (-77, 25, 1000);
    package Colors is new Ada.Numerics.Discrete_Random (Color);
    subtype Upper_Color is Color range Green .. Blue;
    package Upper_Colors is new Ada.Numerics.Discrete_Random (Upper_Color);
    type Mod_Ten is mod 10;
    package Tens is new Ada.Numerics.Discrete_Random (Mod_Ten);
    subtype High is Word range 2 ** 63 .. Word'Last;
    package High_Words is new Ada.Numerics.Discrete_Random (High);
    subtype Singleton is Integer range -7 .. -7;
    package Single is new Ada.Numerics.Discrete_Random (Singleton);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    function Draw (Gen : FR.Generator) return FR.Uniformly_Distributed renames FR.Random;
    type Sampler is access function (Gen : FR.Generator) return FR.Uniformly_Distributed;
    Indirect : Sampler := FR.Random'Access;
    generic
        type Item is (<>);
    procedure Generic_Check;
    procedure Generic_Check is
        package R is new Ada.Numerics.Discrete_Random (Item);
        G : R.Generator;
        X : Item;
    begin
        R.Reset (G, 17);
        for I in 1 .. 30 loop
            X := R.Random (G);
            Check (X >= Item'First and X <= Item'Last);
        end loop;
    end Generic_Check;
    procedure Generic_Color is new Generic_Check (Upper_Color);
    procedure Generic_Word is new Generic_Check (Word);
    procedure Run is
        A, B : FR.Generator;
        X, Y : Dice.Generator;
        Z : Other_Dice.Generator;
        High_Gen : High_Words.Generator;
        One : Single.Generator;
        Ten : Tens.Generator;
        Upper : Upper_Colors.Generator;
        SA, SB, Copy : FR.State;
        DS : Dice.State;
        Numbers : array (1 .. 128) of Float;
        Counts : array (Die) of Natural := (others => 0);
        Failed : Boolean := False;
        Seen_Negative, Seen_Positive, Seen_High, Seen_Low : Boolean := False;
        SG : Signed.Generator;
        UG : Unsigned.Generator;
        type Gen_Ref is access FR.Generator;
        procedure Free is new Ada.Unchecked_Deallocation (FR.Generator, Gen_Ref);
        Heap : Gen_Ref := new FR.Generator;
        Generators : array (1 .. 3) of FR.Generator;
        Calls : Natural := 0;
        function Seed return Integer is
        begin
            Calls := Calls + 1;
            return -129;
        end Seed;
    begin
        FR.Save (A, SA);
        FR.Save (B, SB);
        Check (SA = SB and FR.Image (SA)'Length = FR.Max_Image_Width);
        FR.Reset (B, 0);
        FR.Save (B, SB);
        Check (SA = SB);
        FR.Reset (A, Seed);
        Check (Calls = 1);
        FR.Reset (B, -129);
        for I in Numbers'Range loop
            Numbers (I) := Draw (A);
            Check (Indirect (B) = Numbers (I));
        end loop;
        FR.Save (A, SA);
        Copy := FR.Value (FR.Image (SA));
        Check (Copy = SA);
        FR.Reset (A, Copy);
        FR.Save (A, SB);
        Check (SB = SA);
        for I in 1 .. 20 loop
            Check (FR.Random (A) = FR.Random (B));
        end loop;
        FR.Reset (A, Integer'First);
        FR.Save (A, SA);
        FR.Reset (A, Integer'Last);
        FR.Save (A, SB);
        Check (SA /= SB);
        FR.Reset (A);
        FR.Save (A, SA);
        FR.Reset (A);
        FR.Save (A, SB);
        Check (SA /= SB);
        FR.Save (A, SA);
        declare
            Bad : String := FR.Image (SA);
            Rebased : String (9 .. 8 + FR.Max_Image_Width) := Bad;
        begin
            Check (FR.Value (Rebased) = SA);
            Bad (Bad'First) := '!';
            begin
                Copy := FR.Value (Bad);
            exception
                when Constraint_Error => Failed := True;
            end;
            Check (Failed);
            Bad := "AIDA-RNG1:" & String'(1 .. 64 => '0');
            Failed := False;
            begin
                Copy := FR.Value (Bad);
            exception
                when Constraint_Error => Failed := True;
            end;
            Check (Failed);
        end;
        Failed := False;
        begin
            Copy := FR.Value ("");
        exception
            when Constraint_Error => Failed := True;
        end;
        Check (Failed);
        FR.Save (A, SB);
        Check (SA = SB);
        Dice.Reset (X, -71);
        Dice.Reset (Y, -71);
        Other_Dice.Reset (Z, -71);
        Dice.Save (X, DS);
        for I in 1 .. 1000 loop
            declare
                N : constant Die := Dice.Random (X);
            begin
                Check (N = Dice.Random (Y) and N = Other_Dice.Random (Z));
                Counts (N) := Counts (N) + 1;
            end;
        end loop;
        for N in Die loop Check (Counts (N) > 0); end loop;
        Dice.Reset (X, Dice.Value (Dice.Image (DS)));
        Dice.Reset (Y, DS);
        for I in 1 .. 30 loop Check (Dice.Random (X) = Dice.Random (Y)); end loop;
        Signed.Reset (SG, 12);
        Unsigned.Reset (UG, 12);
        for I in 1 .. 1000 loop
            declare
                S : constant Long_Integer := Signed.Random (SG);
                U : constant Word := Unsigned.Random (UG);
                H : constant High := High_Words.Random (High_Gen);
                T : constant Mod_Ten := Tens.Random (Ten);
                Color : constant Upper_Color := Upper_Colors.Random (Upper);
            begin
                Seen_Negative := Seen_Negative or S < 0;
                Seen_Positive := Seen_Positive or S >= 0;
                Seen_High := Seen_High or U >= 2 ** 63;
                Seen_Low := Seen_Low or U < 2 ** 63;
                Check (H >= 2 ** 63 and T <= 9 and Color /= Red and Single.Random (One) = -7);
            end;
        end loop;
        Check (Seen_Negative and Seen_Positive and Seen_High and Seen_Low);
        FR.Reset (Heap.all, 7);
        FR.Save (Heap.all, SA);
        Free (Heap);
        Check (Heap = null);
        FR.Reset (Generators (2), SA);
        FR.Reset (B, SA);
        Check (FR.Random (Generators (2)) = FR.Random (B));
        Generic_Color;
        Generic_Word;
        declare
            Low : Integer := 2;
            High : Integer := 5;
            subtype Captured is Integer range Low .. High;
            package R is new Ada.Numerics.Discrete_Random (Captured);
            G : R.Generator;
        begin
            Low := 20;
            High := 50;
            for I in 1 .. 100 loop
                declare
                    N : constant Captured := R.Random (G);
                begin
                    Check (N >= 2 and N <= 5);
                end;
            end loop;
        end;
    end Run;
    procedure Dynamic_Check (First, Last : Integer) is
        subtype Interval is Integer range First .. Last;
        package R is new Ada.Numerics.Discrete_Random (Interval);
        G : R.Generator;
        S : R.State;
        N : Interval;
    begin
        Check (First <= Last);
        R.Reset (G, 3);
        R.Save (G, S);
        N := R.Random (G);
        R.Reset (G, S);
        Check (N = R.Random (G));
        for I in 1 .. 100 loop
            N := R.Random (G);
            Check (N >= First and N <= Last);
        end loop;
    end Dynamic_Check;
    F : FR.Generator;
    D : Dice.Generator;
    S : Signed.Generator;
    U : Unsigned.Generator;
    C : Colors.Generator;
    Saved : Dice.State;
    N : Die;
    Value : Float;
    Shared : FR.Generator;
    protected Results is
        procedure Draw;
        procedure Done;
        function Count return Natural;
    private
        Value : Natural := 0;
    end Results;
    protected body Results is
        procedure Draw is
            Sample : Float;
        begin
            Sample := FR.Random (Shared);
            Check (Sample >= 0.0 and Sample < 1.0);
        end Draw;
        procedure Done is begin Value := Value + 1; end Done;
        function Count return Natural is begin return Value; end Count;
    end Results;
    task type Worker;
    task body Worker is
    begin
        Run;
        Dynamic_Check (-71, -3);
        for I in 1 .. 100 loop Results.Draw; end loop;
        Results.Done;
    end Worker;
begin
    Run;
    Dynamic_Check (-9, 9);
    declare
        Failed : Boolean := False;
    begin
        begin
            declare
                subtype Empty is Integer range 3 .. 2;
                package Invalid is new Ada.Numerics.Discrete_Random (Empty);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => Failed := True;
        end;
        Check (Failed);
        Failed := False;
        begin
            Dynamic_Check (5, 4);
        exception
            when Constraint_Error => Failed := True;
        end;
        Check (Failed);
    end;
    FR.Reset (F, 123);
    Dice.Reset (D, -123);
    Signed.Reset (S, Integer'First);
    Unsigned.Reset (U, Integer'Last);
    Dice.Save (D, Saved);
    N := Dice.Random (D);
    Dice.Reset (D, Dice.Value (Dice.Image (Saved)));
    if Dice.Random (D) /= N then raise Program_Error; end if;
    for I in 1 .. 100 loop
        Value := FR.Random (F);
        if Value < 0.0 or Value >= 1.0 then raise Program_Error; end if;
        N := Dice.Random (D);
        declare
            A : Long_Integer := Signed.Random (S);
            B : Word := Unsigned.Random (U);
            Item : Color := Colors.Random (C);
        begin
            null;
        end;
    end loop;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Put_Line ("random library ok");
end RandomLibrary;
