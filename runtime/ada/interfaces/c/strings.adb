with System;
with System.Address_To_Access_Conversions;

package body Interfaces.C.Strings is
    package Char_Addresses is new System.Address_To_Access_Conversions (char);

    function Allocate_Copy (Data : System.Address; Count : size_t) return chars_ptr;
    pragma Import (C, Allocate_Copy, "__ada_c_strings_allocate_copy");
    function Bounded_Length (Item : chars_ptr; Limit : size_t) return size_t;
    pragma Import (C, Bounded_Length, "__ada_c_strings_bounded_length");
    procedure Copy_At (Item : chars_ptr; Offset : size_t; Data : System.Address; Count : size_t);
    pragma Import (C, Copy_At, "__ada_c_strings_copy_at");

    function C_Strlen (Item : chars_ptr) return size_t;
    pragma Import (C, C_Strlen, "strlen");
    procedure C_Free (Item : chars_ptr);
    pragma Import (C, C_Free, "free");
    procedure Copy_Bytes (Target : System.Address; Source : chars_ptr; Count : size_t);
    pragma Import (C, Copy_Bytes, "memmove");

    function To_Chars_Ptr (Item : char_array_access; Nul_Check : Boolean := False) return chars_ptr is
    begin
        if Item = null then
            return Null_Ptr;
        end if;
        if Nul_Check and then not Is_Nul_Terminated (Item.all) then
            raise Terminator_Error;
        end if;
        return chars_ptr (Char_Addresses.To_Pointer (Item.all'Address));
    end To_Chars_Ptr;

    function New_Char_Array (Chars : char_array) return chars_ptr is
        Result : constant chars_ptr := Allocate_Copy (Chars'Address, Chars'Length);
    begin
        if Result = Null_Ptr then
            raise Storage_Error;
        end if;
        return Result;
    end New_Char_Array;

    function New_String (Str : String) return chars_ptr is
    begin
        return New_Char_Array (To_C (Str));
    end New_String;

    procedure Free (Item : in out chars_ptr) is
    begin
        C_Free (Item);
        Item := Null_Ptr;
    end Free;

    function Strlen (Item : chars_ptr) return size_t is
    begin
        if Item = Null_Ptr then
            raise Dereference_Error;
        end if;
        return C_Strlen (Item);
    end Strlen;

    function Read_Length (Item : chars_ptr; Limit : size_t) return size_t is
    begin
        if Item = Null_Ptr then
            raise Dereference_Error;
        end if;
        if Limit = 0 then
            raise Constraint_Error;
        end if;
        return Bounded_Length (Item, Limit);
    end Read_Length;

    function Copy_Array (Item : chars_ptr; Count : size_t) return char_array is
    begin
        return Result : char_array (0 .. Count - 1) do
            Copy_Bytes (Result'Address, Item, Count);
        end return;
    end Copy_Array;

    function Copy_String (Item : chars_ptr; Count : size_t) return String is
    begin
        -- C char and Standard.Character use the same byte for each position.
        return Result : String (1 .. Natural (Count)) do
            if Count /= 0 then
                Copy_Bytes (Result'Address, Item, Count);
            end if;
        end return;
    end Copy_String;

    function Value (Item : chars_ptr) return char_array is
    begin
        return Copy_Array (Item, Strlen (Item) + 1);
    end Value;

    function Value (Item : chars_ptr; Length : size_t) return char_array is
        Count : constant size_t := Read_Length (Item, Length);
    begin
        if Count < Length then
            return Copy_Array (Item, Count + 1);
        end if;
        return Copy_Array (Item, Count);
    end Value;

    function Value (Item : chars_ptr) return String is
    begin
        return Copy_String (Item, Strlen (Item));
    end Value;

    function Value (Item : chars_ptr; Length : size_t) return String is
    begin
        return Copy_String (Item, Read_Length (Item, Length));
    end Value;

    procedure Update (Item : chars_ptr; Offset : size_t; Chars : char_array; Check : Boolean := True) is
        Count : constant size_t := Chars'Length;
        Length : size_t;
    begin
        if Item = Null_Ptr then
            raise Dereference_Error;
        end if;
        if Check then
            Length := Strlen (Item);
            if Offset > Length or else Count > Length - Offset then
                raise Update_Error;
            end if;
        end if;
        Copy_At (Item, Offset, Chars'Address, Count);
    end Update;

    procedure Update (Item : chars_ptr; Offset : size_t; Str : String; Check : Boolean := True) is
    begin
        Update (Item, Offset, To_C (Str, Append_Nul => False), Check);
    end Update;
end Interfaces.C.Strings;
