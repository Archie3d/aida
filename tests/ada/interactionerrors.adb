procedure InteractionErrors is
    type Vector is array (Integer range <>) of Integer;
    type Other is array (Integer range <>) of Integer;
    generic
        type Item is array (Integer range <>) of Integer;
    procedure Change (value : in out Item);
    procedure Change (value : in out Item) is
    begin
        null;
    end Change;
    subtype Pair is Other (1 .. 2);
    procedure Instance is new Change (Other);
    procedure Alias (target : in out Other) renames Instance;
    constantValue : constant Vector := (1, 2);
    function Make return Vector is
    begin
        return (1, 2);
    end Make;
begin
    Alias (target => Pair (constantValue));
    Alias (target => Pair (Make));
end InteractionErrors;
