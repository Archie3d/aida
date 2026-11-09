with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure TaskEntryTypes is
    type Code is (First, Last);
    for Code use (First => 4, Last => 19);
    type Word is mod 2 ** 64;
    type Pointer is access all Integer;
    type Callback is access function (X : Integer) return Integer;
    function Twice (X : Integer) return Integer is begin return X * 2; end Twice;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    type Item (N : Positive) is record Text : String (1 .. N); end record;
    type Root is tagged record N : Integer := 0; end record;
    type Child is new Root with record M : Integer := 0; end record;
    type Resource is new Ada.Finalization.Limited_Controlled with record N : Integer := 0; end record;
    protected type Guard is
        procedure Set;
        function Get return Integer;
    private N : Integer := 0;
    end Guard;
    protected body Guard is
        procedure Set is begin N := 7; end Set;
        function Get return Integer is begin return N; end Get;
    end Guard;
    generic
        type Element is private;
        Initial : Element;
    package Channel is
        task type Writer is entry Send (Output : out Element); end Writer;
    end Channel;
    package body Channel is
        task body Writer is
        begin
            accept Send (Output : out Element) do Output := Initial; end Send;
        end Writer;
    end Channel;
    package Integers is new Channel (Integer, 17);
    subtype Three is String (1 .. 3);
    package Texts is new Channel (Three, "abc");
    IntegerWriter : Integers.Writer;
    TextWriter : Texts.Writer;
    GenericInteger : Integer := 0;
    GenericText : Three := "old";
    task type Mate is entry Touch; end Mate;
    task body Mate is begin accept Touch; end Mate;
    T : Mate;
    task type Server is
        entry Update (A : in out Float; B : in out Long_Float; C : in out Code;
            D : in out Word; P : in out Pointer; F : Callback; Grid : in out Matrix;
            Object : in out Item; Wide : in out Root'Class; R : in out Resource; G : in out Guard; TaskObject : in out Mate);
        entry Overload (X : out Integer);
        entry Overload (X : out Float);
        entry Defaulted (N : Integer := 12);
    end Server;
    task body Server is
    begin
        accept Update (A : in out Float; B : in out Long_Float; C : in out Code;
            D : in out Word; P : in out Pointer; F : Callback; Grid : in out Matrix;
            Object : in out Item; Wide : in out Root'Class; R : in out Resource; G : in out Guard; TaskObject : in out Mate)
        do
            A := A + 1.5;
            B := B + 2.5;
            C := Last;
            D := D + 1;
            P.all := F (P.all);
            Grid (Grid'First (1), Grid'Last (2)) := 23;
            Object.Text := "abc";
            Wide.N := 11;
            R.N := 15;
            G.Set;
            TaskObject.Touch;
        end Update;
        accept Overload (X : out Integer) do X := 8; end Overload;
        accept Overload (X : out Float) do X := 9.0; end Overload;
        accept Defaulted (N : Integer := 12) do
            if Defaulted.N /= 12 then raise Program_Error; end if;
        end Defaulted;
    end Server;
    S : Server;
    A : Float := 1.0;
    B : Long_Float := 2.0;
    C : Code := First;
    D : Word := Word'Last;
    Value : aliased Integer := 6;
    P : Pointer := Value'Access;
    Grid : Matrix (3 .. 4, 7 .. 8) := (others => (others => 0));
    Object : Item (3) := (N => 3, Text => "old");
    Wide : Child;
    R : Resource;
    G : Guard;
    I : Integer;
    F : Float;
begin
    S.Update (A, B, C, D, P, Twice'Access, Grid, Object, Wide, R, G, T);
    S.Overload (I);
    S.Overload (F);
    S.Defaulted;
    IntegerWriter.Send (GenericInteger);
    TextWriter.Send (GenericText);
    if GenericInteger /= 17 or GenericText /= "abc" then raise Program_Error; end if;
    if A /= 2.5 or B /= 4.5 or C /= Last or D /= 0 or Value /= 12
        or Grid (3, 8) /= 23 or Object.Text /= "abc" or Wide.N /= 11
        or R.N /= 15 or G.Get /= 7 or I /= 8 or F /= 9.0
    then raise Program_Error; end if;
    Put_Line ("entry types OK");
end TaskEntryTypes;
