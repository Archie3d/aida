with Ada.Text_IO;
with Ada.Strings.Unbounded;
with Ada.Strings.Wide_Unbounded;
with Ada.Strings.Wide_Wide_Unbounded;
procedure UnboundedEquality is
    generic
        type Value (<>) is private;
    function Same (Left, Right : Value) return Boolean;
    function Same (Left, Right : Value) return Boolean is
    begin
        return Left = Right;
    end Same;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    procedure Check_1 is
        package U renames Ada.Strings.Unbounded;
        use type U.Unbounded_String;
        type Items is array (Integer range <>) of U.Unbounded_String;
        type Matrix is array (1 .. 2, 1 .. 2) of U.Unbounded_String;
        type Holder is record
            Item : U.Unbounded_String;
        end record;
        type Nested is record
            Value : Holder;
            Extra : Items (1 .. 2);
        end record;
        type Holders is array (1 .. 2) of Holder;
        type Tagged_Holder is tagged record
            Item : U.Unbounded_String;
        end record;
        function Same_Value is new Same (U.Unbounded_String);
        function Same_Items is new Same (Items);
        function Same_Holder is new Same (Holder);
        Text : String (7 .. 8) := (Character'Val (233), Character'Val (120));
        Normalized : String (1 .. 2) := Text;
        A : Items (1 .. 3) := (U.Null_Unbounded_String, U.To_Unbounded_String (Text), U.To_Unbounded_String ("tail"));
        B : Items (-2 .. 0) := (U.To_Unbounded_String (""), U.To_Unbounded_String (Normalized), U.To_Unbounded_String ("tail"));
        Copy : Items := A;
        Empty_A : Items (1 .. 0);
        Empty_B : Items (5 .. 4);
        H : Holder := (Item => U.To_Unbounded_String (Text));
        J : Holder := (Item => U.To_Unbounded_String (Text));
        N : Nested := (Value => H, Extra => (U.To_Unbounded_String (""), U.To_Unbounded_String (Text)));
        M : Nested := (Value => J, Extra => (U.Null_Unbounded_String, U.To_Unbounded_String (Text)));
        L : Holders := (others => H);
        R : Holders := (others => J);
        T : Tagged_Holder := (Item => U.To_Unbounded_String (Text));
        V : Tagged_Holder := (Item => U.To_Unbounded_String (Text));
        Grid : Matrix := ((A (1), A (2)), (A (2), A (3)));
        Other : Matrix := ((B (-2), B (-1)), (B (-1), B (0)));
    begin
        Check (A = B and not (A /= B) and A = Copy);
        Check (Same_Items (A, B) and Same_Value (A (2), B (-1)));
        Check (Empty_A = Empty_B and Same_Items (Empty_A, Empty_B));
        Check (Same_Value (U.Null_Unbounded_String, U.To_Unbounded_String ("")));
        Check (H = J and Same_Holder (H, J) and N = M and L = R and T = V and Grid = Other);
        for Iteration in 1 .. 50 loop
            Check (A = B and N = M and Grid = Other);
        end loop;
        U.Replace_Element (B (-1), 2, Character'Val (121));
        Check (A /= B and not Same_Items (A, B));
        Check (U.To_String (A (2)) = Text and Copy = A);
        B (-1) := U.To_Unbounded_String (Text);
        Check (A = B);
        U.Append (B (-1), Character'Val (120));
        Check (A /= B);
        B (-1) := U.To_Unbounded_String (Text);
        B (-2) := U.To_Unbounded_String ("nonempty");
        Check (A /= B);
        B (-2) := U.Null_Unbounded_String;
        Check (A = B);
    end Check_1;
    procedure Check_2 is
        package U renames Ada.Strings.Wide_Unbounded;
        use type U.Unbounded_Wide_String;
        type Items is array (Integer range <>) of U.Unbounded_Wide_String;
        type Matrix is array (1 .. 2, 1 .. 2) of U.Unbounded_Wide_String;
        type Holder is record
            Item : U.Unbounded_Wide_String;
        end record;
        type Nested is record
            Value : Holder;
            Extra : Items (1 .. 2);
        end record;
        type Holders is array (1 .. 2) of Holder;
        type Tagged_Holder is tagged record
            Item : U.Unbounded_Wide_String;
        end record;
        function Same_Value is new Same (U.Unbounded_Wide_String);
        function Same_Items is new Same (Items);
        function Same_Holder is new Same (Holder);
        Text : Wide_String (7 .. 8) := (Wide_Character'Val (955), Wide_Character'Val (120));
        Normalized : Wide_String (1 .. 2) := Text;
        A : Items (1 .. 3) := (U.Null_Unbounded_Wide_String, U.To_Unbounded_Wide_String (Text), U.To_Unbounded_Wide_String ("tail"));
        B : Items (-2 .. 0) := (U.To_Unbounded_Wide_String (""), U.To_Unbounded_Wide_String (Normalized), U.To_Unbounded_Wide_String ("tail"));
        Copy : Items := A;
        Empty_A : Items (1 .. 0);
        Empty_B : Items (5 .. 4);
        H : Holder := (Item => U.To_Unbounded_Wide_String (Text));
        J : Holder := (Item => U.To_Unbounded_Wide_String (Text));
        N : Nested := (Value => H, Extra => (U.To_Unbounded_Wide_String (""), U.To_Unbounded_Wide_String (Text)));
        M : Nested := (Value => J, Extra => (U.Null_Unbounded_Wide_String, U.To_Unbounded_Wide_String (Text)));
        L : Holders := (others => H);
        R : Holders := (others => J);
        T : Tagged_Holder := (Item => U.To_Unbounded_Wide_String (Text));
        V : Tagged_Holder := (Item => U.To_Unbounded_Wide_String (Text));
        Grid : Matrix := ((A (1), A (2)), (A (2), A (3)));
        Other : Matrix := ((B (-2), B (-1)), (B (-1), B (0)));
    begin
        Check (A = B and not (A /= B) and A = Copy);
        Check (Same_Items (A, B) and Same_Value (A (2), B (-1)));
        Check (Empty_A = Empty_B and Same_Items (Empty_A, Empty_B));
        Check (Same_Value (U.Null_Unbounded_Wide_String, U.To_Unbounded_Wide_String ("")));
        Check (H = J and Same_Holder (H, J) and N = M and L = R and T = V and Grid = Other);
        for Iteration in 1 .. 50 loop
            Check (A = B and N = M and Grid = Other);
        end loop;
        U.Replace_Element (B (-1), 2, Wide_Character'Val (121));
        Check (A /= B and not Same_Items (A, B));
        Check (U.To_Wide_String (A (2)) = Text and Copy = A);
        B (-1) := U.To_Unbounded_Wide_String (Text);
        Check (A = B);
        U.Append (B (-1), Wide_Character'Val (120));
        Check (A /= B);
        B (-1) := U.To_Unbounded_Wide_String (Text);
        B (-2) := U.To_Unbounded_Wide_String ("nonempty");
        Check (A /= B);
        B (-2) := U.Null_Unbounded_Wide_String;
        Check (A = B);
    end Check_2;
    procedure Check_3 is
        package U renames Ada.Strings.Wide_Wide_Unbounded;
        use type U.Unbounded_Wide_Wide_String;
        type Items is array (Integer range <>) of U.Unbounded_Wide_Wide_String;
        type Matrix is array (1 .. 2, 1 .. 2) of U.Unbounded_Wide_Wide_String;
        type Holder is record
            Item : U.Unbounded_Wide_Wide_String;
        end record;
        type Nested is record
            Value : Holder;
            Extra : Items (1 .. 2);
        end record;
        type Holders is array (1 .. 2) of Holder;
        type Tagged_Holder is tagged record
            Item : U.Unbounded_Wide_Wide_String;
        end record;
        function Same_Value is new Same (U.Unbounded_Wide_Wide_String);
        function Same_Items is new Same (Items);
        function Same_Holder is new Same (Holder);
        Text : Wide_Wide_String (7 .. 8) := (Wide_Wide_Character'Val (128578), Wide_Wide_Character'Val (120));
        Normalized : Wide_Wide_String (1 .. 2) := Text;
        A : Items (1 .. 3) := (U.Null_Unbounded_Wide_Wide_String, U.To_Unbounded_Wide_Wide_String (Text), U.To_Unbounded_Wide_Wide_String ("tail"));
        B : Items (-2 .. 0) := (U.To_Unbounded_Wide_Wide_String (""), U.To_Unbounded_Wide_Wide_String (Normalized), U.To_Unbounded_Wide_Wide_String ("tail"));
        Copy : Items := A;
        Empty_A : Items (1 .. 0);
        Empty_B : Items (5 .. 4);
        H : Holder := (Item => U.To_Unbounded_Wide_Wide_String (Text));
        J : Holder := (Item => U.To_Unbounded_Wide_Wide_String (Text));
        N : Nested := (Value => H, Extra => (U.To_Unbounded_Wide_Wide_String (""), U.To_Unbounded_Wide_Wide_String (Text)));
        M : Nested := (Value => J, Extra => (U.Null_Unbounded_Wide_Wide_String, U.To_Unbounded_Wide_Wide_String (Text)));
        L : Holders := (others => H);
        R : Holders := (others => J);
        T : Tagged_Holder := (Item => U.To_Unbounded_Wide_Wide_String (Text));
        V : Tagged_Holder := (Item => U.To_Unbounded_Wide_Wide_String (Text));
        Grid : Matrix := ((A (1), A (2)), (A (2), A (3)));
        Other : Matrix := ((B (-2), B (-1)), (B (-1), B (0)));
    begin
        Check (A = B and not (A /= B) and A = Copy);
        Check (Same_Items (A, B) and Same_Value (A (2), B (-1)));
        Check (Empty_A = Empty_B and Same_Items (Empty_A, Empty_B));
        Check (Same_Value (U.Null_Unbounded_Wide_Wide_String, U.To_Unbounded_Wide_Wide_String ("")));
        Check (H = J and Same_Holder (H, J) and N = M and L = R and T = V and Grid = Other);
        for Iteration in 1 .. 50 loop
            Check (A = B and N = M and Grid = Other);
        end loop;
        U.Replace_Element (B (-1), 2, Wide_Wide_Character'Val (121));
        Check (A /= B and not Same_Items (A, B));
        Check (U.To_Wide_Wide_String (A (2)) = Text and Copy = A);
        B (-1) := U.To_Unbounded_Wide_Wide_String (Text);
        Check (A = B);
        U.Append (B (-1), Wide_Wide_Character'Val (120));
        Check (A /= B);
        B (-1) := U.To_Unbounded_Wide_Wide_String (Text);
        B (-2) := U.To_Unbounded_Wide_Wide_String ("nonempty");
        Check (A /= B);
        B (-2) := U.Null_Unbounded_Wide_Wide_String;
        Check (A = B);
    end Check_3;
    procedure Exercise is
    begin
        Check_1;
        Check_2;
        Check_3;
    end Exercise;
    task type Worker;
    task body Worker is
    begin
        Exercise;
    end Worker;
begin
    Exercise;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("unbounded equality passed");
end UnboundedEquality;
