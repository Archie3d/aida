package RuntimeArrayLibraryErrors is
    N : Integer := 3;
    subtype Text is String (1 .. N);
    type Vector is array (1 .. N) of Integer;
    Missing : String;
    type Holder is record
        A : Text;
    end record;
    type Rows is array (1 .. 2) of Vector;
    type Pointer is access Text;
    Bad : Pointer := new Text;
    Bits : Integer := Text'Size;
    subtype Again is Text (1 .. N);
end RuntimeArrayLibraryErrors;
