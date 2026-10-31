package body GenericDiscriminantLibrary is
    function Score (X : T) return Index is
    begin
        return X.First + X.Last;
    end Score;
    function Matches (X : T) return Boolean is
    begin
        return X in Selected;
    end Matches;
end GenericDiscriminantLibrary;
