with Interfaces.C.Strings;
package C_String_Defaults is
    pragma Preelaborate;
    Pointer : Interfaces.C.Strings.chars_ptr;
    Vector : Interfaces.C.Strings.chars_ptr_array (7 .. 8);
end C_String_Defaults;

with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Interfaces.C;
with Interfaces.C.Strings;
with C_String_Defaults;

procedure InterfacesCStrings is
    package C renames Interfaces.C;
    package S renames Interfaces.C.Strings;
    use type C.char;
    use type C.char_array;
    use type C.size_t;
    use type S.chars_ptr;
    use type S.char_array_access;

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;

    function Find (Item : S.chars_ptr; Unit : C.int) return S.chars_ptr;
    pragma Import (C, Find, "strchr");
    procedure Free_Array is new Ada.Unchecked_Deallocation (C.char_array, S.char_array_access);

    procedure Check_Value (Item : S.chars_ptr; Expected : String) is
        Text : constant String := S.Value (Item);
        Bytes : constant C.char_array := S.Value (Item);
    begin
        Check (Text'First = 1 and then Text = Expected);
        Check (Bytes'First = 0 and then Bytes = C.To_C (Expected));
        Check (S.Strlen (Item) = C.size_t (Expected'Length));
    end Check_Value;

    procedure Check_Owned is
        Top : constant C.size_t := C.size_t'Last;
        High_Byte : constant Character := Character'Val (16#FF#);
        Source : C.char_array (Top - 2 .. Top) := ('A', C.nul, C.To_C (High_Byte));
        Empty : C.char_array (10 .. 9);
        Text : S.chars_ptr := S.New_Char_Array (Source);
        Blank : S.chars_ptr := S.New_Char_Array (Empty);
        No_Nul : S.chars_ptr := S.New_Char_Array (Source (Top .. Top));
        Embedded : S.chars_ptr := S.New_String ("D" & Character'Val (0) & "E");
        Null_String : S.chars_ptr := S.New_String ("");
        Prefix : constant C.char_array := S.Value (No_Nul, 1);
        Bounded : constant C.char_array := S.Value (No_Nul, C.size_t'Last);
        Bounded_Text : constant String := S.Value (No_Nul, 1);
    begin
        Source (Top - 2) := 'Z';
        Check_Value (Text, "A");
        Check_Value (Blank, "");
        Check_Value (No_Nul, String'(1 => High_Byte));
        Check_Value (Embedded, "D");
        Check_Value (Null_String, "");
        begin
            declare
                Invalid : C.char_array := S.Value (Text, 0);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                Invalid : String := S.Value (Text, 0);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        S.Free (Text);
        S.Free (Text);
        S.Free (Blank);
        S.Free (No_Nul);
        S.Free (Embedded);
        S.Free (Null_String);
        Check (Text = S.Null_Ptr and then Blank = S.Null_Ptr and then No_Nul = S.Null_Ptr
            and then Embedded = S.Null_Ptr and then Null_String = S.Null_Ptr);
        Check (Prefix'First = 0 and then Prefix'Last = 0 and then Prefix (0) = C.To_C (High_Byte));
        Check (Bounded'First = 0 and then Bounded'Last = 1 and then Bounded (1) = C.nul);
        Check (Bounded_Text'First = 1 and then Bounded_Text = String'(1 => High_Byte));
    end Check_Owned;

    procedure Check_Borrowed is
        Top : constant C.size_t := C.size_t'Last;
        Buffer : aliased C.char_array := C.char_array'(Top - 3 .. Top => C.nul);
        Missing : aliased C.char_array := C.char_array'(12 => 'X', 13 => 'Y');
        Empty : C.char_array (7 .. 6);
        Borrowed : S.chars_ptr;
        Ada_Heap : S.char_array_access := new C.char_array'(C.To_C (String'("pool")));
        View : S.chars_ptr := S.To_Chars_Ptr (Ada_Heap);
        Suffixed : S.chars_ptr;
    begin
        Buffer (Top - 3) := 'a';
        Buffer (Top - 2) := 'b';
        Buffer (Top - 1) := 'c';
        Borrowed := S.To_Chars_Ptr (Buffer'Unchecked_Access, Nul_Check => True);
        Suffixed := Find (Borrowed, C.int (Character'Pos ('b')));
        Check (S.To_Chars_Ptr (null, Nul_Check => True) = S.Null_Ptr);
        Check_Value (View, "pool");
        Free_Array (Ada_Heap);
        Check (Ada_Heap = null);
        Check_Value (Borrowed, "abc");
        Check_Value (Suffixed, "bc");
        Check (S.Value (S.To_Chars_Ptr (Missing'Unchecked_Access), 2) = String'("XY"));
        begin
            View := S.To_Chars_Ptr (Missing'Unchecked_Access, Nul_Check => True);
            raise Program_Error;
        exception
            when C.Terminator_Error => null;
        end;
        -- Borrowed pointers share the original bytes in both directions.
        Buffer (Top - 3) := 'X';
        S.Update (Borrowed, 1, Buffer (Top - 3 .. Top - 2));
        Check (Buffer (Top - 1) = 'b' and then Buffer (Top) = C.nul);
        Check_Value (Borrowed, "XXb");
        S.Update (Borrowed, 3, Empty);
        S.Update (Borrowed, 2, String'("Z"), Check => False);
        Check (Buffer (Top - 1) = 'Z' and then Buffer (Top) = C.nul);
        begin
            S.Update (Borrowed, C.size_t'Last, String'("oops"));
            raise Program_Error;
        exception
            when S.Update_Error => null;
        end;
        begin
            S.Update (Borrowed, 2, String'("xx"));
            raise Program_Error;
        exception
            when S.Update_Error => null;
        end;
        begin
            S.Update (Borrowed, 4, Empty);
            raise Program_Error;
        exception
            when S.Update_Error => null;
        end;
        begin
            S.Update (Borrowed, 0, String'(""));
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        Check_Value (Borrowed, "XXZ");
        S.Update (Borrowed, 0, String'("A" & Character'Val (0) & "C"));
        Check_Value (Borrowed, "A");
        Check (Buffer (Top - 1) = 'C' and then Buffer (Top) = C.nul);
    end Check_Borrowed;

    procedure Check_Null is
        Text : String (1 .. 1);
        Bytes : C.char_array (0 .. 0);
        Count : C.size_t;
    begin
        for Which in 1 .. 6 loop
            begin
                case Which is
                    when 1 => Text := S.Value (S.Null_Ptr);
                    when 2 => Text := S.Value (S.Null_Ptr, 5);
                    when 3 => Bytes := S.Value (S.Null_Ptr);
                    when 4 => Bytes := S.Value (S.Null_Ptr, 0);
                    when 5 => Count := S.Strlen (S.Null_Ptr);
                    when 6 => S.Update (S.Null_Ptr, 0, String'("x"), Check => False);
                    when others => raise Program_Error;
                end case;
                raise Program_Error;
            exception
                when S.Dereference_Error => null;
            end;
        end loop;
    end Check_Null;

    Shared : aliased C.char_array := C.To_C (String'("shared"));
    Shared_View : constant S.chars_ptr := S.To_Chars_Ptr (Shared'Unchecked_Access);
    Worker_Count : constant := 4;
    protected Result is
        procedure Arrive;
        entry Start;
        procedure Finish;
        function Finished return Natural;
    private
        Arrived, Completed : Natural := 0;
    end Result;
    protected body Result is
        procedure Arrive is
        begin
            Arrived := Arrived + 1;
        end Arrive;
        entry Start when Arrived = Worker_Count is
        begin
            null;
        end Start;
        procedure Finish is
        begin
            Completed := Completed + 1;
        end Finish;
        function Finished return Natural is
        begin
            return Completed;
        end Finished;
    end Result;
    task type Worker;
    task body Worker is
        Item : S.chars_ptr;
    begin
        Result.Arrive;
        Result.Start;
        for Iteration in 1 .. 25 loop
            Item := S.New_String (S.Value (Shared_View, 3));
            S.Update (Item, 1, String'("OL"));
            Check_Value (Item, "sOL");
            S.Free (Item);
        end loop;
        Result.Finish;
    end Worker;
begin
    Check (C_String_Defaults.Pointer = S.Null_Ptr and then C_String_Defaults.Vector (7) = S.Null_Ptr
        and then C_String_Defaults.Vector (8) = S.Null_Ptr);
    Check_Owned;
    Check_Borrowed;
    Check_Null;
    declare
        Workers : array (1 .. Worker_Count) of Worker;
    begin
        null;
    end;
    Check (Result.Finished = Worker_Count);
    Check_Value (Shared_View, "shared");
    Ada.Text_IO.Put_Line ("Interfaces.C.Strings passed");
end InterfacesCStrings;
