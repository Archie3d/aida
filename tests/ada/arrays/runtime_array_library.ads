package Runtime_Array_Library is
    N : Integer := 3;
    subtype Text is String (2 .. N);
    subtype Alias_Text is Text;
    type Vector is array (-1 .. N) of Integer;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    subtype Shape is Matrix (2 .. N, 5 .. N + 3);
    subtype Index is Integer range 1 .. N;
    type Indexed is array (Index) of Integer;
    A : Text := (others => 'a');
    B : String (4 .. N + 2) := "bc";
    V : Vector := (others => 7);
    type Derived is new Vector;
    D : Derived := (others => 8);
    M : Shape := (others => (others => 9));
    I : Indexed := (others => 6);
    Empty : String (5 .. N) := "";
    Inferred : String := A & B;
    View : String renames A;
    function Make return Text;
    procedure Inspect (X : Text; Y : Shape);
    generic
        Count : Positive;
    package Buffer is
        type Vector is array (1 .. Count) of Integer;
        subtype Text is String (1 .. Count);
        A : Vector := (others => Count);
        B : Text := (others => 'b');
    end Buffer;
    package Three is new Buffer (N);
    package Four is new Buffer (N + 1);
    type Hidden is private;
    function Make_Hidden return Hidden;
    procedure Check_Hidden (X : Hidden);
    generic
        X : in String;
        Y : in out String;
    package Capture is
        Snapshot : String := X;
        procedure Check;
    end Capture;
private
    type Hidden is array (1 .. N) of Integer;
    H : Hidden := (others => 8);
end Runtime_Array_Library;
