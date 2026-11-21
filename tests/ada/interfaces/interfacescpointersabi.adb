with Ada.Text_IO;
with Interfaces.C;
with Interfaces.C.Pointers;
with Interfaces.C.Strings;

procedure InterfacesCPointersAbi is
    package C renames Interfaces.C;
    package S renames Interfaces.C.Strings;
    use type C.int;
    use type C.short;
    use type C.ptrdiff_t;
    use type C.size_t;
    use type S.chars_ptr;

    type Axis is (Before, First, Middle, Last, After);
    subtype Window is Axis range First .. Last;
    type Sample is record
        Value : C.int;
        Code : C.short;
    end record;
    for Sample'Size use 128;
    type Samples is array (Window range <>) of aliased Sample;
    package Pointers is new C.Pointers (Window, Sample, Samples, (0, 0));
    use type Pointers.Pointer;

    function Native_Sample (Offset : C.ptrdiff_t) return Pointers.Pointer;
    pragma Import (C, Native_Sample, "aida_c_pointer_sample");
    function Change (Start, Selected : Pointers.Pointer; Distance : C.ptrdiff_t) return C.int;
    pragma Import (C, Change, "aida_c_pointer_change");
    package Name_Pointers is new C.Pointers (C.size_t, S.chars_ptr, S.chars_ptr_array, S.Null_Ptr);
    function Native_Names return Name_Pointers.Pointer;
    pragma Import (C, Native_Names, "aida_c_pointer_names");

    Start : Pointers.Pointer := Native_Sample (0);
    Next : Pointers.Pointer := Start + 1;
    Original : Samples := Pointers.Value (Start);
    Output : Samples (Window) := (others => (-1, -1));
    Names : S.chars_ptr_array := Name_Pointers.Value (Native_Names);

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    Check (Next = Native_Sample (1) and then Native_Sample (2) - Start = 2
        and then Start - Native_Sample (2) = -2 and then Next - 1 = Start);
    Check (Original'First = First and then Original'Last = Last and then Original (Middle) = (9, 20)
        and then Original (Last) = (0, 0));
    Check (Change (Start, Next, Next - Start) = 1 and then Next.all.Value = 31
        and then Original (Middle).Value = 9);
    Pointers.Copy_Terminated_Array (Start, Output (First)'Access, Limit => 2);
    Check (Output (First) = (7, 10) and then Output (Middle) = (31, 20) and then Output (Last) = (-1, -1));
    Pointers.Copy_Array (Next, Output (First)'Access, Length => 2);
    Check (Output (First) = (31, 20) and then Output (Middle) = (0, 0) and then Output (Last) = (-1, -1));
    Check (Names'First = 0 and then Names'Last = 2 and then Names (2) = S.Null_Ptr
        and then S.Value (Names (0)) = String'("first") and then S.Value (Names (1)) = String'("second"));
    Ada.Text_IO.Put_Line ("native C pointer stride passed");
end InterfacesCPointersAbi;
