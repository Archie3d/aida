with System.Storage_Elements;
procedure StorageElementErrors is
    use System.Storage_Elements;
    use type System.Address;
    procedure Take_Offset (Value : Storage_Offset) is
    begin
        null;
    end Take_Offset;
    Element : Storage_Element := 1;
    Offset : Storage_Offset := 0;
    Address : System.Address := System.Null_Address;
    Bits : Integer_Address := 1;
    function Bad (Left, Right : System.Address) return System.Address renames System.Storage_Elements."-";
    function Add (Left : System.Address; Right : Storage_Offset) return System.Address renames System.Storage_Elements."+";
    type Callback is access function (Left : System.Address; Right : Storage_Offset) return System.Address;
    Pointer : Callback := Add'Access;
    type Convert_Callback is access function (Value : Integer_Address) return System.Address;
    Converter : Convert_Callback := To_Address'Access;
    type Bytes is array (Integer range <>) of Storage_Element;
    for Bytes'Component_Size use 16;
    type Duplicate is array (Integer range <>) of Storage_Element;
    for Duplicate'Component_Size use 8;
    for Duplicate'Component_Size use 8;
    type Late is array (Integer range <>) of Storage_Element;
    Frozen : Late (1 .. 2);
    for Late'Component_Size use 8;
    subtype View is Bytes (1 .. 2);
    for View'Component_Size use 8;
    type Dynamic is array (Integer range <>) of Storage_Element;
    Variable : Integer := 8;
    for Dynamic'Component_Size use Variable;
    type Not_Array is range 0 .. 10;
    for Not_Array'Component_Size use 8;
    type Boolean_Array is array (Integer range <>) of Boolean;
    for Boolean_Array'Component_Size use 8;
    pragma Pack (Boolean_Array);
    type Wrong_Expression is array (Integer range <>) of Boolean;
    pragma Pack (Wrong_Expression);
    for Wrong_Expression'Component_Size use True;
    generic
        type Formal_Array is array (Integer range <>) of Storage_Element;
    package Query_Formal is
        Value : constant := Formal_Array'Component_Size;
    end Query_Formal;
    Static_Query : constant := Storage_Array'Component_Size;
    Invalid_Query : Integer := Integer'Component_Size;
    Invalid_Argument : Integer := Bytes'Component_Size (1);
    type Broken is array (1 .. 3) of Missing;
    for Broken'Component_Size use 8;
    Broken_Query : Integer := Broken'Component_Size;
begin
    Take_Offset (Bits);
    Element := Offset;
    Bits := To_Integer (Bits);
    Address := To_Address (Offset);
    Address := Address + Bits;
    Offset := Address + Address;
end StorageElementErrors;
