with Interfaces.C.Strings;
with System.Storage_Elements;

package body Interfaces.C.Pointers is
    package Storage renames System.Storage_Elements;
    use type Storage.Integer_Address;
    use type Storage.Storage_Offset;

    -- The storage helpers operate on the native address in a named pointer.
    function Add_Bytes (Ref : Pointer; Offset : Storage.Storage_Offset) return Pointer;
    pragma Import (C, Add_Bytes, "__ada_storage_add");
    function Subtract_Bytes (Ref : Pointer; Offset : Storage.Storage_Offset) return Pointer;
    pragma Import (C, Subtract_Bytes, "__ada_storage_subtract");
    function Distance_In_Bytes (Left, Right : Pointer) return Storage.Storage_Offset;
    pragma Import (C, Distance_In_Bytes, "__ada_storage_difference");
    function Address_Of (Ref : Pointer) return Storage.Integer_Address;
    pragma Import (C, Address_Of, "__ada_storage_to_integer");

    function Stride return Storage.Storage_Offset is
    begin
        -- Aliased array elements occupy the full object storage size.
        return Element'Max_Size_In_Storage_Elements;
    end Stride;

    function "+" (Left : Pointer; Right : ptrdiff_t) return Pointer is
    begin
        if Left = null then
            raise Pointer_Error;
        end if;
        return Add_Bytes (Left, Storage.Storage_Offset (Right) * Stride);
    end "+";

    function "+" (Left : ptrdiff_t; Right : Pointer) return Pointer is
    begin
        return Right + Left;
    end "+";

    function "-" (Left : Pointer; Right : ptrdiff_t) return Pointer is
    begin
        if Left = null then
            raise Pointer_Error;
        end if;
        return Subtract_Bytes (Left, Storage.Storage_Offset (Right) * Stride);
    end "-";

    function "-" (Left, Right : Pointer) return ptrdiff_t is
    begin
        if Left = null or else Right = null then
            raise Pointer_Error;
        end if;
        if Left = Right then
            return 0;
        end if;
        return ptrdiff_t (Distance_In_Bytes (Left, Right) / Stride);
    end "-";

    procedure Increment (Ref : in out Pointer) is
    begin
        Ref := Ref + 1;
    end Increment;

    procedure Decrement (Ref : in out Pointer) is
    begin
        Ref := Ref - 1;
    end Decrement;

    function Virtual_Length (Ref : Pointer; Terminator : Element := Default_Terminator) return ptrdiff_t is
        Cursor : Pointer := Ref;
        Count : ptrdiff_t := 0;
    begin
        if Ref = null then
            raise Strings.Dereference_Error;
        end if;
        while Cursor.all /= Terminator loop
            Count := Count + 1;
            Increment (Cursor);
        end loop;
        return Count;
    end Virtual_Length;

    function Value (Ref : Pointer; Terminator : Element := Default_Terminator) return Element_Array is
    begin
        return Value (Ref, Length => Virtual_Length (Ref, Terminator) + 1);
    end Value;

    function Value (Ref : Pointer; Length : ptrdiff_t) return Element_Array is
        Upper : Index'Base := Index'First;
    begin
        if Ref = null then
            raise Strings.Dereference_Error;
        end if;
        if Length <= 0 then
            if Upper /= Index'Base'First then
                return Result : Element_Array (Index'First .. Index'Pred (Upper)) do
                    null;
                end return;
            end if;
            if Upper = Index'Base'Last then
                raise Constraint_Error;
            end if;
            return Result : Element_Array (Index'Succ (Upper) .. Index'First) do
                null;
            end return;
        end if;
        if Upper > Index'Last then
            raise Constraint_Error;
        end if;
        -- Discrete positions need not fit in ptrdiff_t, even on a 64-bit target.
        for Position in 2 .. Length loop
            if Upper >= Index'Last then
                raise Constraint_Error;
            end if;
            Upper := Index'Succ (Upper);
        end loop;
        return Result : Element_Array (Index'First .. Upper) do
            Copy_Array (Ref, Result (Index'First)'Unchecked_Access, Length);
        end return;
    end Value;

    procedure Copy_Array (Source, Target : Pointer; Length : ptrdiff_t) is
    begin
        if Source = null or else Target = null then
            raise Strings.Dereference_Error;
        end if;
        if Source = Target or else Length <= 0 then
            return;
        end if;
        -- Copy from the end when writing into higher addresses. Use typed
        -- assignment so element representation and assignment rules apply.
        if Address_Of (Target) > Address_Of (Source) then
            for Offset in reverse 0 .. Length - 1 loop
                (Target + Offset).all := (Source + Offset).all;
            end loop;
        else
            for Offset in 0 .. Length - 1 loop
                (Target + Offset).all := (Source + Offset).all;
            end loop;
        end if;
    end Copy_Array;

    procedure Copy_Terminated_Array (Source, Target : Pointer; Limit : ptrdiff_t := ptrdiff_t'Last;
                                     Terminator : Element := Default_Terminator) is
        Count : ptrdiff_t := 0;
    begin
        if Source = null or else Target = null then
            raise Strings.Dereference_Error;
        end if;
        while Count < Limit loop
            Count := Count + 1;
            exit when (Source + (Count - 1)).all = Terminator;
        end loop;
        Copy_Array (Source, Target, Count);
    end Copy_Terminated_Array;
end Interfaces.C.Pointers;
