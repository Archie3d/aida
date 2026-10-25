procedure ArrayIndexErrors is
    subtype Row is Integer range 1 .. 2;
    type Choice is (First, Middle, Last);
    type Box_First is array (Integer range <>, Integer range 1 .. 2) of Integer;
    type Box_Last is array (Integer range 1 .. 2, Integer range <>) of Integer;
    type Bare_Last is array (Integer range <>, Row) of Integer;
    type Bare_First is array (Row, Integer range <>) of Integer;
    type Raw_First is array (1 .. 2, Integer range <>) of Integer;
    type Raw_Last is array (Integer range <>, 1 .. 2) of Integer;
    type Enum_Mix is array (Choice range <>, Choice range First .. Last) of Boolean;
    type Boolean_Mix is array (Boolean, Boolean range <>) of Character;
    type Three_Boxed is array (Row range <>, Row range <>, Row) of Integer;
    type Alternating is array (Row, Row range <>, Row) of Integer;
    -- Legal declarations interspersed with errors must not get diagnostics.
    type Closed is array (Row, Choice) of Integer;
    type Open is array (Row range <>, Choice range <>) of Integer;
    A : array (Integer range <>, Row) of Integer;
    B : array (Row, Integer range <>) of Integer;
    generic
        type Matrix is array (Row, Choice range <>) of Integer;
    package Bad_Formal_One is
    end Bad_Formal_One;
    generic
        type Matrix is array (Row range <>, Choice) of Integer;
    package Bad_Formal_Two is
    end Bad_Formal_Two;
    type After_Errors is array (Row, Boolean) of Integer;
begin
    null;
end ArrayIndexErrors;
