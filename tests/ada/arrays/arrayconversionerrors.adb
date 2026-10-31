procedure ArrayConversionErrors is
    subtype Small is Integer range 1 .. 3;
    subtype Same_Range is Integer range 1 .. 3;
    type A is array (Integer range <>) of Small;
    type B is array (Integer range <>) of Same_Range;
    type C is array (Integer range <>) of Integer;
    type M is array (Integer range <>, Integer range <>) of Small;
    type Color is (Red, Green, Blue);
    type E is array (Color range <>) of Small;
    Source : A (1 .. 2) := (1, 2);
    Constant_Source : constant A := (1, 2);
    Bad_Component : B := B (Source);
    Bad_Base : C := C (Source);
    Bad_Rank : M := M (Source);
    Bad_Index : E := E (Source);
    Bad_Aggregate : C := C ((1, 2));
    Bad_Literal : String := String ("ab");
    function Make return A is
    begin
        return (1, 2);
    end Make;
    procedure Change (X : in out A) is
    begin
        null;
    end Change;
    type Tagged_Item is tagged record
        Value : Integer;
    end record;
    type Tagged_A is array (Integer range <>) of Tagged_Item;
    type Tagged_B is array (Integer range <>) of Tagged_Item;
    Tagged_Source : Tagged_A (1 .. 1);
    procedure Change_Tagged (X : in out Tagged_B) is
    begin
        null;
    end Change_Tagged;
    package Hidden is
        type Token is private;
    private
        type Token is new Integer;
    end Hidden;
    type Tokens is array (Integer range <>) of Hidden.Token;
    type Other_Tokens is array (Integer range <>) of Hidden.Token;
    Private_Source : Tokens (1 .. 1);
    procedure Change_Private (X : in out Other_Tokens) is
    begin
        null;
    end Change_Private;
begin
    Change_Private (Other_Tokens (Private_Source));
    Change (A (Constant_Source));
    Change (A (Make));
    Change_Tagged (Tagged_B (Tagged_Source));
    A (Source) := Source;
    A (Source) (1) := 1;
    Change (A (A (Source)));
end ArrayConversionErrors;
