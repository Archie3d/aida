package body DefaultDiscriminantLibrary is
    procedure Set (Target : in out Item; Value : Item) is
        procedure Nested is
        begin
            Target := Value;
        end Nested;
    begin
        Nested;
    end Set;
    procedure Fill (Target : out Item) is
    begin
        Target := (True, 42);
    end Fill;
    function Is_Constrained (Target : Item) return Boolean is
    begin
        return Target'Constrained;
    end Is_Constrained;
end DefaultDiscriminantLibrary;
