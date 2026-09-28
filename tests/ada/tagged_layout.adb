package body Tagged_Layout is
    procedure Reset (Item : out Root) is
    begin
        Item := (X => 8);
    end Reset;
    function Make return Root is
    begin
        return (X => 9);
    end Make;
end Tagged_Layout;
