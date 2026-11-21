with Interfaces.C;
with Interfaces.C.Pointers;
procedure InterfacesCPointerErrors is
    package C renames Interfaces.C;
    package Pointers is new C.Pointers (C.size_t, C.char, C.char_array, C.nul);
    type Motion is access procedure (Ref : in out Pointers.Pointer);
    type Addition is access function (Left : Pointers.Pointer; Right : C.ptrdiff_t) return Pointers.Pointer;
    Next : Motion := Pointers.Increment'Access;
    Add : Addition := Pointers."+"'Access;
    type Plain_Array is array (C.size_t range <>) of C.char;
    package Missing_Aliasing is new C.Pointers (C.size_t, C.char, Plain_Array, C.nul);
begin
    null;
end InterfacesCPointerErrors;
