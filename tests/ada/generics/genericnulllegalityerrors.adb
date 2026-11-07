procedure GenericNullLegalityErrors is
    generic
        with function Get return Integer is null;
    package Bad_Function is
    end Bad_Function;
    generic
        type T is tagged private;
        with procedure Set (X : in out T) is abstract null;
    package Bad_Abstract is
    end Bad_Abstract;
    generic
        with procedure Action is null;
    package Good_Default is
        procedure Alias renames Action;
    end Good_Default;
    package Instance is new Good_Default;
    type Callback is access procedure;
    Bad_Access : Callback := Instance.Alias'Access;
    Bad_Value : Boolean := 1;
begin
    null;
end GenericNullLegalityErrors;
