package Array_Index_Model is
    type Index is range -3 .. 3;
    subtype Row is Index range -1 .. 1;
    type Choice is (First, Last);
    type Closed is array (Row, Choice) of Integer;
    type Open is array (Row range <>, Choice range <>) of Integer;
    type Explicit_Ranges is array (Index range -1 .. 1, Choice range First .. Last) of Integer;
    type Bare_Subtypes is array (Row, Choice, Boolean) of Integer;
    type Boxed_Subtypes is array (Row range <>, Choice range <>, Boolean range <>) of Integer;
    type Raw_Ranges is array (1 .. 2, 'A' .. 'B') of Integer;
    type Mixed_Notations is array (Row, Choice range First .. Last) of Integer;
    Limit : Integer := 2;
    type Dynamic is array (1 .. Limit, Boolean) of Integer;
end Array_Index_Model;
