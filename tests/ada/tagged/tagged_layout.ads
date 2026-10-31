package Tagged_Layout is
    type Root is tagged record
        X : Integer := 7;
    end record;
    procedure Reset (Item : out Root);
    function Make return Root;
end Tagged_Layout;
