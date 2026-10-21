with Concat_Private;
with Concat_Controlled_Checks;
package body Concat_Checks is
    procedure Run is
        use type Concat_Private.Items;
        Private_Pair : Concat_Private.Items := Concat_Private.Make (11) & Concat_Private.Make (22);
        Private_Trio : Concat_Private.Items := Private_Pair & Concat_Private.Make (33);
        type Index is range -20 .. 20;
        type Vector is array (Index range <>) of Integer;
        A : Vector (-4 .. -2) := (1, 2, 3);
        B : Vector (7 .. 8) := (4, 5);
        Empty_Left : Vector (9 .. 2);
        Empty_Right : Vector (12 .. 1);
        AB : Vector := A & B;
        AC : Vector := A & 6;
        CB : Vector := 0 & B;
        CC : Vector := 7 & 8;
        EB : Vector := Empty_Left & B;
        AE : Vector := A & Empty_Right;
        EE : Vector := Empty_Left & Empty_Right;
        EC : Vector := Empty_Left & 9;
        CE : Vector := 9 & Empty_Right;
        Chain : Vector := (Empty_Left & A) & B & Empty_Right;
        type Fixed is array (Index range 5 .. 7) of Integer;
        F : Fixed := (10, 20, 30);
        Fixed_Result : Fixed := F (6 .. 7) & 40;
        type Derived is new Vector (5 .. 7);
        D : Derived := (10, 20, 30);
        Derived_Result : Vector := Vector (D (6 .. 7) & 40);
        type Word is range 0 .. 65535;
        for Word'Size use 16;
        type Words is array (Positive range <>) of Word;
        W : Words (5 .. 6) := (16#1234#, 16#ABCD#);
        WW : Words := W & Word'(16#FFFF#);
        type Longs is array (Positive range <>) of Long_Integer;
        L : Longs (3 .. 4) := (5_000_000_000, -5_000_000_000);
        LL : Longs := Long_Integer'(7_000_000_000) & L;
        type Reals is array (Positive range <>) of Long_Float;
        R : Reals := Long_Float'(1.25) & Long_Float'(-2.5);
        type Color is (Red, Green, Blue);
        type Colors is array (Positive range <>) of Color;
        Palette : Colors := Red & Blue;
        type Row is array (4 .. 5) of Integer;
        type Rows is array (Positive range <>) of Row;
        First_Row : Row := (10, 20);
        Second_Row : Row := (30, 40);
        Table : Rows := First_Row & Second_Row;
        More_Rows : Rows := Table & First_Row;
        type Item is record
            Number : Long_Integer;
            Flag : Boolean;
        end record;
        type Items is array (Positive range <>) of Item;
        One : Item := (Number => 100, Flag => True);
        Two : Item := (Number => 200, Flag => False);
        Pair : Items := One & Two;
        Trio : Items := One & Pair;
        type Link is access Integer;
        type Links is array (Positive range <>) of Link;
        P : Link := new Integer'(42);
        Pointers : Links := P & Link'(null);
        type Day is (Mon, Tue, Wed, Thu, Fri);
        type Schedule is array (Day range <>) of Integer;
        S : Schedule (Wed .. Thu) := (10, 20);
        Extended : Schedule := S & 30;
        Both : Schedule := 10 & 20;
        type Wide_Vector is array (Long_Integer range <>) of Integer;
        Wide : Wide_Vector (5_000_000_000 .. 5_000_000_001) := (1, 2);
        Wide_Result : Wide_Vector := Wide & 3;
        Low_Bound : Integer := 5;
        High_Bound : Integer := 7;
        type Runtime_Fixed is array (Low_Bound .. High_Bound) of Integer;
        Runtime_Source : Runtime_Fixed := (10, 20, 30);
        Runtime_Result : Runtime_Fixed := Runtime_Source (6 .. 7) & 40;
        subtype Dynamic_Index is Integer range Low_Bound .. High_Bound;
        type Dynamic_Vector is array (Dynamic_Index range <>) of Integer;
        Dynamic_Pair : Dynamic_Vector := 10 & 20;
        Calls : Integer := 0;
        function Once return Vector is
        begin
            Calls := Calls + 1;
            return A;
        end Once;
        function Element return Integer is
        begin
            Calls := Calls + 1;
            return 8;
        end Element;
        function Join (Left, Right : Vector) return Vector is
        begin
            return Left & Right;
        end Join;
        Dynamic_Result : Vector := Join (A, B);
        Once_Result : Vector := Once & Element;
        Explicit_Result : Vector := "&" (1, 2);
        subtype Tiny is Integer range 1 .. 3;
        type Tiny_Vector is array (Positive range <>) of Tiny;
        Bad : Integer := 9;
        Tiny_Target : Tiny_Vector (1 .. 2) := (1, 2);
        Limit : Index := 20;
        Edge : Vector (Limit .. Limit) := (others => 1);
        generic
            type Element_Type is private;
            type Sequence is array (Positive range <>) of Element_Type;
        function Append (Left : Sequence; Right : Element_Type) return Sequence;
        function Append (Left : Sequence; Right : Element_Type) return Sequence is
        begin
            return Left & Right;
        end Append;
        type Numbers is array (Positive range <>) of Integer;
        function Append_Number is new Append (Integer, Numbers);
        Numbers_Source : Numbers (5 .. 6) := (10, 20);
        Generic_Result : Numbers := Append_Number (Numbers_Source, 30);
        type Custom is array (Positive range <>) of Integer;
        Custom_Calls : Integer := 0;
        function "&" (Left, Right : Custom) return Custom is
        begin
            Custom_Calls := Custom_Calls + 1;
            return Left;
        end "&";
        Custom_Source : Custom (7 .. 8) := (10, 20);
        Custom_Result : Custom := Custom_Source & Custom_Source;
        type Text is array (Index range <>) of Character;
        T : Text := "ab" & 'c';
    begin
        Concat_Controlled_Checks.Run;
        if Concat_Private.Number (Private_Pair (1)) /= 11
            or Concat_Private.Number (Private_Trio (3)) /= 33 then
            raise Program_Error;
        end if;
        if Runtime_Result (5) /= 20 or Runtime_Result (7) /= 40
            or Dynamic_Pair'First /= 5 or Dynamic_Pair'Last /= 6 then
            raise Program_Error;
        end if;
        if Generic_Result'First /= 5 or Generic_Result (7) /= 30
            or Custom_Calls /= 1 or Custom_Result'First /= 7 or Custom_Result'Length /= 2 then
            raise Program_Error;
        end if;
        if AB'First /= -4 or AB'Last /= 0 or AB (-2) /= 3 or AB (0) /= 5
            or AC'First /= -4 or AC'Last /= -1 or AC (-1) /= 6
            or CB'First /= Index'First or CB (Index'First + 1) /= 4
            or CC'First /= Index'First or CC (Index'First + 1) /= 8
            or EB'First /= B'First or EB'Last /= B'Last or EB /= B
            or AE'First /= A'First or AE'Last /= A'Last or AE /= A
            or EE'First /= Empty_Right'First or EE'Last /= Empty_Right'Last or EE'Length /= 0
            or EC'First /= Index'First or EC (Index'First) /= 9
            or CE'First /= Index'First or CE (Index'First) /= 9
            or Chain /= AB or Chain'First /= A'First
            or Fixed_Result (5) /= 20 or Fixed_Result (7) /= 40
            or Derived_Result'First /= 6 or Derived_Result'Last /= 8
            or WW'First /= 5 or WW (7) /= 16#FFFF# or WW (6) /= 16#ABCD#
            or LL'First /= 1 or LL (2) /= 5_000_000_000
            or R (1) /= 1.25 or R (2) /= -2.5 or Palette (2) /= Blue
            or Table (1) (4) /= 10 or More_Rows (3) (5) /= 20
            or Pair (2).Number /= 200 or Trio (1).Flag /= True or Trio (3).Flag /= False
            or Pointers (1).all /= 42 or Pointers (2) /= null
            or Extended'First /= Wed or Extended'Last /= Fri or Extended (Fri) /= 30
            or Both'First /= Mon or Both'Last /= Tue
            or Wide_Result'First /= 5_000_000_000 or Wide_Result (5_000_000_002) /= 3
            or Dynamic_Result /= AB or Once_Result (-1) /= 8 or Calls /= 2
            or Explicit_Result'First /= Index'First or Explicit_Result (Index'First + 1) /= 2
            or T'First /= Index'First or T (Index'First + 2) /= 'c'
        then
            raise Program_Error;
        end if;
        begin
            Tiny_Target := Tiny'(1) & Bad;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        if Tiny_Target (1) /= 1 or Tiny_Target (2) /= 2 then
            raise Program_Error;
        end if;
        begin
            declare
                Overflow : Vector := Edge & 2;
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        -- Resetting the outer lower bound must not hide the inner overflow.
        begin
            declare
                Overflow : Vector := 1 & (Edge & 2);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
    end Run;
end Concat_Checks;
