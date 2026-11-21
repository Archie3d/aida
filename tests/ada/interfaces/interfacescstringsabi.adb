with Ada.Text_IO;
with Interfaces.C;
with Interfaces.C.Strings;

procedure InterfacesCStringsAbi is
    package C renames Interfaces.C;
    package S renames Interfaces.C.Strings;
    use type C.char_array;
    use type C.int;
    use type C.size_t;
    use type S.chars_ptr;

    procedure Fail_Next;
    pragma Import (C, Fail_Next, "aida_c_strings_fail_next");
    function Last_Request return C.size_t;
    pragma Import (C, Last_Request, "aida_c_strings_last_request");
    function Raw (Terminated : C.int) return S.chars_ptr;
    pragma Import (C, Raw, "aida_c_strings_raw");
    procedure Release_Raw (Item : S.chars_ptr);
    pragma Import (C, Release_Raw, "aida_c_strings_release_raw");
    procedure Set_Last (Vector : in out S.chars_ptr_array; Count : C.size_t;
                        Expected : S.chars_ptr; Status : out C.int);
    pragma Import (C, Set_Last, "aida_c_strings_set_last");

    Top : constant C.size_t := C.size_t'Last;
    Buffer : aliased C.char_array := C.char_array'(8 => 'a', 9 => C.nul, 10 => 'b');
    Borrowed : S.chars_ptr;
    Text : S.chars_ptr := S.New_String ("first");
    Vector : S.chars_ptr_array (Top - 1 .. Top);
    type Slot_Access is access all S.chars_ptr;
    Last_Slot : Slot_Access := Vector (Top)'Access;
    Status : C.int;

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
begin
    Check (Last_Request = 6);
    Fail_Next;
    Borrowed := S.To_Chars_Ptr (Buffer'Unchecked_Access, Nul_Check => True);
    begin
        Text := S.New_Char_Array (Buffer);
        raise Program_Error;
    exception
        when Storage_Error =>
            Check (Last_Request = 2 and then S.Value (Text) = String'("first"));
    end;
    Fail_Next;
    begin
        Text := S.New_String ("new");
        raise Program_Error;
    exception
        when Storage_Error =>
            Check (Last_Request = 4 and then S.Value (Text) = String'("first"));
    end;
    S.Free (Text);
    Text := S.New_String ("new");
    Check (S.Value (Text) = String'("new"));
    S.Free (Text);

    -- C owns these exact-size buffers. The counted reads must stay within two bytes.
    Text := Raw (0);
    Check (S.Value (Text, 2) = String'("xy"));
    Check (S.Value (Text, 2) = C.To_C (String'("xy"), Append_Nul => False));
    Release_Raw (Text);
    Text := Raw (1);
    Check (S.Value (Text, C.size_t'Last) = String'("x"));
    Check (S.Value (Text, C.size_t'Last) = C.To_C (String'("x")));
    Release_Raw (Text);

    -- The C callee receives char ** and updates the aliased pointer slot in place.
    Vector (Top - 1) := Borrowed;
    Set_Last (Vector, 2, Borrowed, Status);
    Check (Status = 1);
    Check (Last_Slot.all = Vector (Top) and then S.Value (Last_Slot.all) = String'("native"));
    Check (S.Value (Borrowed) = String'("a"));
    Ada.Text_IO.Put_Line ("native C string ABI passed");
end InterfacesCStringsAbi;
