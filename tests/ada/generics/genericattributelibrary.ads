generic
    type T is (<>);
    with function Next (Object : T) return T is T'Succ;
    with function Encode (T : T) return String is T'Image;
    with function Decode (Text : String) return T is T'Value;
package GenericAttributeLibrary is
    function Following (Object : T) return T renames Next;
    function Step (Object : T) return T;
    function Roundtrip (Object : T) return T;
    generic
        type U is (<>);
        with function Forwarded (Object : T) return T is Next;
        with function Previous (Object : U) return U is U'Pred;
        with function Outer_Image (Object : T) return String is T'Image;
    package Inner is
        function Advance (Object : T) return T renames Forwarded;
        function Check (Object : T; Other : U) return Boolean;
    end Inner;
end GenericAttributeLibrary;
