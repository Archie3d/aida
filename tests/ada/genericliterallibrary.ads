package GenericLiteralLibrary is
    type Color is (Red, Green, Blue);
    type Glyph is ('A', 'a');
    generic
        type T is new Color;
        with function Red return T is Red;
        with function Letter return Glyph is GenericLiteralLibrary.'a';
    package Views is
        function Value return T renames Red;
        function Character_Value return Glyph renames Letter;
        generic
            with function Forwarded return T is Red;
            with function Green return T is Green;
        package Inner is
            function Value return T renames Forwarded;
            function Second return T renames Green;
        end Inner;
    end Views;
end GenericLiteralLibrary;
