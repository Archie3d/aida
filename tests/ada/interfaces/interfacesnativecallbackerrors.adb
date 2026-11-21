with Interfaces.C;
procedure InterfacesNativeCallbackErrors is
    package C renames Interfaces.C;
    type Small is range -4 .. 3;
    type Packed is array (Positive range <>) of Small;
    pragma Pack (Packed);
    pragma Convention (C, Packed);
    type Array_Ptr is access all C.char_array;
    pragma Convention (C, Array_Ptr);
    type Bad_Parameter is access procedure (Value : Packed);
    pragma Convention (C, Bad_Parameter);
    type Bad_Result is access function return C.char_array;
    pragma Convention (C, Bad_Result);
    type Protected_Callback is access protected procedure (Value : C.int);
    pragma Convention (C, Protected_Callback);
    type Writer is access procedure (Value : C.char_array);
    pragma Convention (C, Writer);
    procedure Helper (Value : C.char_array);
    pragma Import (C, Helper, "__ada_test_array_helper");
    Callback : Writer := Helper'Access;
begin
    null;
end InterfacesNativeCallbackErrors;
