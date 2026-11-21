with Interfaces.C;
procedure InterfacesNativeImportErrors is
    package C renames Interfaces.C;
    type Small is range -4 .. 3;
    type Packed is array (Positive range <>) of Small;
    pragma Pack (Packed);
    type Matrix is array (Positive range <>, Positive range <>) of C.int;
    type Item is tagged record
        Value : C.int;
    end record;
    type Array_Ptr is access all C.char_array;
    type Ada_Callback is access procedure;

    function Return_Array return C.char_array;
    pragma Import (C, Return_Array, "return_array");
    function Return_Record return Item;
    pragma Import (C, Return_Record, "return_record");
    function Return_Anonymous return access C.int;
    pragma Import (C, Return_Anonymous, "return_pointer");
    procedure Bad_Array (Value : Packed);
    pragma Import (C, Bad_Array, "bad_array");
    procedure Bad_Rank (Value : Matrix);
    pragma Import (C, Bad_Rank, "bad_rank");
    procedure Bad_Record (Value : in out Item);
    pragma Import (C, Bad_Record, "bad_record");
    procedure Bad_Pointer (Value : Array_Ptr);
    pragma Import (C, Bad_Pointer, "bad_pointer");
    procedure Bad_Callback (Value : Ada_Callback);
    pragma Import (C, Bad_Callback, "bad_callback");
    type Late_Items is array (Positive range <>) of Small;
    pragma Convention (C, Late_Items);
    type Late_Callback is access procedure (Value : Late_Items);
    pragma Convention (C, Late_Callback);
    pragma Pack (Late_Items);
    procedure Bad_Layout (Value : Late_Items);
    pragma Import (C, Bad_Layout, "bad_layout");
begin
    null;
end InterfacesNativeImportErrors;
