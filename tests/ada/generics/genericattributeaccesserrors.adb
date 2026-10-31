procedure GenericAttributeAccessErrors is
    generic
        with function Encode (Object : Integer) return String;
    package Views is
        function Renamed (Object : Integer) return String renames Encode;
    end Views;
    package Instance is new Views (Integer'Image);
    type Callback is access function (Object : Integer) return String;
    Call : Callback := Instance.Renamed'Access;
    generic
        with function F (Object : Integer) return String is Integer'Image;
    package Outer is
        package Inner is new Views (F);
    end Outer;
    package Nested is new Outer;
    Other : Callback := Nested.Inner.Renamed'Access;
begin
    null;
end GenericAttributeAccessErrors;
