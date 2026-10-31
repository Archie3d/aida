package DefaultDiscriminantLibrary is
    type Item (D : Boolean := False) is record
        Value : Integer := 1;
    end record;
    Global : Item;
    procedure Set (Target : in out Item; Value : Item);
    procedure Fill (Target : out Item);
    function Is_Constrained (Target : Item) return Boolean;
end DefaultDiscriminantLibrary;
