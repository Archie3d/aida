procedure GenericNullAccessErrors is
    generic
        with procedure Hook (Object : Integer) is null;
    package Views is
        procedure Renamed (Object : Integer) renames Hook;
    end Views;
    package Default_View is new Views;
    type Callback is access procedure (Object : Integer);
    Call : Callback := Default_View.Renamed'Access;
    generic
        with procedure Action (Object : Integer);
    package Forward is
        procedure Renamed (Object : Integer) renames Action;
    end Forward;
    package Outer is
        generic
            with procedure Hook (Object : Integer) is null;
        package Nested is
            package Inner is new Forward (Hook);
        end Nested;
    end Outer;
    package Instance is new Outer.Nested;
    Other : Callback := Instance.Inner.Renamed'Access;
begin
    null;
end GenericNullAccessErrors;
