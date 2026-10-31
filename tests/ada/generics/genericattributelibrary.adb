package body GenericAttributeLibrary is
    function Step (Object : T) return T is
    begin
        return Next (Object => Object);
    end Step;
    function Roundtrip (Object : T) return T is
    begin
        return Decode (Encode (Object));
    end Roundtrip;
    package body Inner is
        function Check (Object : T; Other : U) return Boolean is
        begin
            return Outer_Image (Object) = T'Image (Object)
                and U'Succ (Previous (Other)) = Other;
        end Check;
    end Inner;
end GenericAttributeLibrary;
