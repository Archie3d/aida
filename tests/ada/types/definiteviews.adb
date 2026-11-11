with Ada.Text_IO;
procedure Definiteviews is
    package Model is
        subtype Side is Positive range 1 .. 4;
        type Vector is array (Positive range <>) of Integer;
        type Root is tagged record
            Flag : Boolean := True;
        end record;
        type Hidden_Indefinite (<>) is new Root with private;
        type Hidden_Defaulted (<>) is private;
        function Make_Indefinite (Size : Side) return Hidden_Indefinite;
        function Make_Defaulted (Size : Side) return Hidden_Defaulted;
        function Size_Of (Item : Hidden_Indefinite) return Side;
        function Size_Of (Item : Hidden_Defaulted) return Side;
        procedure Check_Full_Views;
    private
        type Hidden_Indefinite (Size : Side) is new Root with record
            Value : Integer := Size;
        end record;
        type Hidden_Defaulted (Size : Side := 2) is record
            Value : Integer := Size;
        end record;
    end Model;
    package body Model is
        function Make_Indefinite (Size : Side) return Hidden_Indefinite is
            Item : Hidden_Indefinite (Size);
        begin
            return Item;
        end Make_Indefinite;
        function Make_Defaulted (Size : Side) return Hidden_Defaulted is
            Item : Hidden_Defaulted (Size);
        begin
            return Item;
        end Make_Defaulted;
        function Size_Of (Item : Hidden_Indefinite) return Side is
        begin
            return Item.Size;
        end Size_Of;
        function Size_Of (Item : Hidden_Defaulted) return Side is
        begin
            return Item.Size;
        end Size_Of;
        -- The full view permits uninitialized objects and components when
        -- every discriminant has a default; the public unknown view does not.
        Defaulted : Hidden_Defaulted;
        Initialized : Hidden_Indefinite := Make_Indefinite (3);
        type Default_Array is array (1 .. 2) of Hidden_Defaulted;
        Defaults : Default_Array;
        type Constrained_Array is array (1 .. 2) of Hidden_Indefinite (2);
        Constrained : Constrained_Array;
        procedure Check_Full_Views is
        begin
            if Defaulted.Value /= 2 or Initialized.Value /= 3
                or Defaults (1).Value /= 2 or Defaults (2).Value /= 2
                or Constrained (1).Value /= 2 or Constrained (2).Value /= 2 then
                raise Program_Error with "private full-view initialization";
            end if;
        end Check_Full_Views;
    end Model;
    -- Initializers make objects of the public unknown-discriminant views legal.
    A : Model.Hidden_Indefinite := Model.Make_Indefinite (4);
    B : Model.Hidden_Defaulted := Model.Make_Defaulted (3);
    Fixed : Model.Vector (1 .. 3) := (others => 2);
    Initialized : Model.Vector := (1, 2, 3);
    Anonymous : array (Positive range <>) of Integer := (1, 2, 3, 4);
    type Holder is record
        Items : Model.Vector (1 .. 2) := (others => 5);
    end record;
    subtype Pair is Model.Vector (1 .. 2);
    type Pairs is array (1 .. 2) of Pair;
    Values : Pairs := (others => (others => 6));
    Object : Holder;
    type Roots is array (1 .. 2) of Model.Root;
    Tagged_Objects : Roots;
    Classwide : Model.Root'Class := Model.Root'(Flag => True);
    protected type Gate (Size : Positive) is
        function Value return Positive;
    end Gate;
    protected body Gate is
        function Value return Positive is
        begin
            return Size;
        end Value;
    end Gate;
    task type Worker (Size : Positive) is
        entry Read (Value : out Positive);
    end Worker;
    task body Worker is
    begin
        accept Read (Value : out Positive) do
            Value := Size;
        end Read;
    end Worker;
    G : Gate (7);
    W : Worker (9);
    Task_Value : Positive := 1;
begin
    Model.Check_Full_Views;
    W.Read (Task_Value);
    if Model.Size_Of (A) /= 4 or Model.Size_Of (B) /= 3
        or Fixed'Length /= 3 or Initialized'Length /= 3 or Anonymous'Length /= 4
        or Fixed (3) /= 2 or Initialized (3) /= 3 or Anonymous (4) /= 4
        or Object.Items (2) /= 5 or Values (2) (2) /= 6
        or not Tagged_Objects (1).Flag or not Classwide.Flag
        or G.Value /= 7 or Task_Value /= 9 then
        raise Program_Error with "initialized and constrained objects";
    end if;
    Ada.Text_IO.Put_Line ("definite views passed");
end Definiteviews;
