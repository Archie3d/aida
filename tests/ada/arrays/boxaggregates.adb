with Ada.Text_IO;
procedure BoxAggregates is
    Calls : Integer := 0;
    N : Integer := 3;
    function Next return Integer is
    begin
        Calls := Calls + 1;
        return Calls;
    end Next;
    type Link is access Integer;
    type Item is record
        Left, Right : Integer := Next;
        Flag : Boolean := True;
        Pointer : Link;
    end record;
    type Items is array (Integer range <>) of Item;
    type Matrix is array (Integer range <>, Integer range <>) of Item;
    type Links is array (Integer range <>) of Link;
    subtype Row is Items (1 .. N);
    type Holder is record
        Data : Row;
        Text : String (1 .. N) := (others => 'x');
    end record;
    type Tagged_Item is tagged record
        Value : Integer := 42;
    end record;
    type Extension is new Tagged_Item with record
        Extra : Boolean := True;
    end record;
    A : Item := (Left | Right | Flag | Pointer => <>);
    B : Item := (Left => 99, others => <>);
    C : Items (4 .. 6) := (4 | 6 => <>, 5 => (others => <>));
    D : Matrix (1 .. 2, 3 .. 4) := (others => (others => <>));
    E : Holder := (others => <>);
    P : Links (1 .. N) := (1 => new Integer'(7), others => <>);
    X : Extension := (Tagged_Item with others => <>);
    Y : Item := (1, 2, True, null, others => <>);
    Z : Items := (7 .. 8 => <>);
    Empty : Items := (N .. N - 1 => <>);
    generic
        Seed : Integer;
    package Defaults is
        type R is record
            Value : Integer := Seed;
        end record;
        Object : R := (others => <>);
    end Defaults;
    package First_Default is new Defaults (11);
    package Second_Default is new Defaults (22);
    Before : Integer := Calls;
begin
    if A.Left + A.Right /= 3 or not A.Flag or A.Pointer /= null
        or B.Left /= 99 or B.Right /= 3 or not B.Flag or B.Pointer /= null
        or First_Default.Object.Value /= 11 or Second_Default.Object.Value /= 22
        or Calls /= 27 or X.Value /= 42 or not X.Extra or Y.Right /= 2
        or Z'First /= 7 or Z'Length /= 2 or Empty'Length /= 0
        or P(1).all /= 7 or P(2) /= null or P(3) /= null
        or E.Text /= "xxx" or E.Data'Length /= N then
        raise Program_Error;
    end if;
    A.Pointer := new Integer'(8);
    A := (others => <>);
    if A.Left + A.Right /= 2 * Before + 3 or A.Pointer /= null then
        raise Program_Error;
    end if;
    P := (others => <>);
    if P(1) /= null or P(2) /= null or P(3) /= null then raise Program_Error; end if;
    C := (others => <>);
    if Calls /= Before + 8 or C(4).Pointer /= null then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("box aggregates ok");
end BoxAggregates;
