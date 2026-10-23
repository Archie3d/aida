package body DiscriminantData is
    procedure Set (Target : in out Buffer; Source : Buffer) is
    begin
        Target := Source;
    end Set;
    function Copy (X : Buffer) return Buffer is
    begin
        return X;
    end Copy;
end DiscriminantData;
