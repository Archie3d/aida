package Interfaces.C.Strings is
    pragma Preelaborate (Strings);

    type char_array_access is access all char_array;
    type chars_ptr is private;
    pragma Preelaborable_Initialization (chars_ptr);

    type chars_ptr_array is array (size_t range <>) of aliased chars_ptr;
    Null_Ptr : constant chars_ptr;

    function To_Chars_Ptr (Item : char_array_access; Nul_Check : Boolean := False) return chars_ptr;
    function New_Char_Array (Chars : char_array) return chars_ptr;
    function New_String (Str : String) return chars_ptr;
    procedure Free (Item : in out chars_ptr);

    Dereference_Error : exception;

    function Value (Item : chars_ptr) return char_array;
    function Value (Item : chars_ptr; Length : size_t) return char_array;
    function Value (Item : chars_ptr) return String;
    function Value (Item : chars_ptr; Length : size_t) return String;
    function Strlen (Item : chars_ptr) return size_t;

    procedure Update (Item : chars_ptr; Offset : size_t; Chars : char_array; Check : Boolean := True);
    procedure Update (Item : chars_ptr; Offset : size_t; Str : String; Check : Boolean := True);

    Update_Error : exception;

private
    type chars_ptr is access all char;
    pragma Convention (C, chars_ptr);
    for chars_ptr'Storage_Size use 0;
    Null_Ptr : constant chars_ptr := null;
end Interfaces.C.Strings;
