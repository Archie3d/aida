with Ada.Text_IO;
with Interfaces.C;
with Interfaces.C.Pointers;
with Interfaces.C.Strings;

procedure InterfacesCPointers is
    package C renames Interfaces.C;
    use type C.char;
    use type C.char_array;
    use type C.size_t;
    use type C.ptrdiff_t;

    Top : constant C.size_t := C.size_t'Last;

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;

    procedure Check_Characters is
        package Char_Pointers is new C.Pointers (C.size_t, C.char, C.char_array, C.nul);
        use type Char_Pointers.Pointer;
        function Native_Length (Ref : Char_Pointers.Pointer) return C.size_t;
        pragma Import (C, Native_Length, "strlen");
        Source : C.char_array (Top - 4 .. Top) := ('a', 'b', 'c', C.nul, 'Z');
        Target : C.char_array (10 .. 14) := (others => '!');
        Start : Char_Pointers.Pointer := Source (Top - 4)'Access;
        Next : Char_Pointers.Pointer := 1 + Start;
        Last : Char_Pointers.Pointer := Start + 4;
        Past_End : Char_Pointers.Pointer := Last + 1;
        Terminated : C.char_array := Char_Pointers.Value (Start);
        Counted : C.char_array := Char_Pointers.Value (Start, Length => 5);
        Custom : C.char_array := Char_Pointers.Value (Start, Terminator => 'b');
        Empty : C.char_array := Char_Pointers.Value (Start, Length => 0);
        Negative : C.char_array := Char_Pointers.Value (Start, Length => -1);
    begin
        Check (Start + 0 = Start and then Start - Start = 0
            and then Next = Source (Top - 3)'Access and then Last = Source (Top)'Access);
        Check (Last - Start = 4 and then Start - Last = -4
            and then Last - 3 = Next and then Next + (-1) = Start and then Start - (-1) = Next
            and then Past_End - Start = 5 and then Past_End - 1 = Last);
        Check (Native_Length (Next) = 2 and then Char_Pointers.Virtual_Length (Start) = 3
            and then Char_Pointers.Virtual_Length (Start, Terminator => 'b') = 1);
        Check (Terminated'First = 0 and then Terminated'Last = 3
            and then Terminated = C.To_C (String'("abc")));
        Check (Counted'First = 0 and then Counted'Last = 4
            and then Counted (3) = C.nul and then Counted (4) = 'Z');
        Check (Custom'First = 0 and then Custom'Last = 1 and then Custom (1) = 'b');
        Check (Empty'Length = 0 and then Negative'Length = 0);
        Char_Pointers.Increment (Next);
        Check (Next = Source (Top - 2)'Access);
        Char_Pointers.Decrement (Next);
        Check (Next = Source (Top - 3)'Access);

        Char_Pointers.Copy_Terminated_Array (Start, Target (11)'Access, Limit => 2);
        Check (Target = C.char_array'('!', 'a', 'b', '!', '!'));
        Char_Pointers.Copy_Terminated_Array (Start, Target (10)'Access, Terminator => 'b');
        Check (Target = C.char_array'('a', 'b', 'b', '!', '!'));
        Char_Pointers.Copy_Terminated_Array (Start, Target (10)'Access);
        Char_Pointers.Copy_Terminated_Array (Start, Target (14)'Access, Limit => 0);
        Char_Pointers.Copy_Array (Start, Target (14)'Access, Length => -1);
        Check (Target = C.char_array'('a', 'b', 'c', C.nul, '!'));
        Char_Pointers.Copy_Terminated_Array (Last, Target (14)'Access, Limit => 1);
        Check (Target (14) = 'Z');

        Char_Pointers.Copy_Array (Start, Next, Length => 4);
        Check (Source = C.char_array'('a', 'a', 'b', 'c', C.nul));
        Char_Pointers.Copy_Terminated_Array (Next, Start);
        Check (Source = C.char_array'('a', 'b', 'c', C.nul, C.nul));
        Next.all := 'Y';
        Check (Source (Top - 3) = 'Y' and then Terminated (1) = 'b');
    end Check_Characters;

    procedure Check_Modular_Indices is
        use type C.unsigned_short;
        subtype High_Index is C.size_t range Top - 2 .. Top;
        type Words is array (High_Index range <>) of aliased C.unsigned_short;
        package Word_Pointers is new C.Pointers (High_Index, C.unsigned_short, Words, 0);
        use type Word_Pointers.Pointer;
        Source : Words (High_Index) := (16#FEDC#, 16#8000#, 0);
        Start : Word_Pointers.Pointer := Source (Top - 2)'Access;
        Last : Word_Pointers.Pointer := Start + 2;
        Data : Words := Word_Pointers.Value (Start);
        Empty : Words := Word_Pointers.Value (Start, Length => 0);
    begin
        Check (Last = Source (Top)'Access and then Last - Start = 2 and then Last.all = 0);
        Check (Data'First = Top - 2 and then Data'Last = Top and then Data = Source);
        Check (Empty'Length = 0 and then Empty'First = Top - 2);
        begin
            declare
                Invalid : Words := Word_Pointers.Value (Start, Length => 4);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        begin
            Last := Start + C.ptrdiff_t'Last;
            raise Program_Error;
        exception
            when Constraint_Error => Check (Last = Source (Top)'Access);
        end;
    end Check_Modular_Indices;

    procedure Check_Private_Element is
        type Item (Tag : Boolean) is record
            Number : Integer;
        end record;
        subtype Fixed_Item is Item (True);
        subtype Origin is Integer range -3 .. -1;
        type Items is array (Origin range <>) of aliased Fixed_Item;
        package Item_Pointers is new C.Pointers (Origin, Fixed_Item, Items, (True, 0));
        use type Item_Pointers.Pointer;
        Source : Items (Origin) := ((True, 4), (True, 5), (True, 0));
        Start : Item_Pointers.Pointer := Source (-3)'Access;
        Snapshot : Items := Item_Pointers.Value (Start, Length => 2);
    begin
        Check (Snapshot'First = -3 and then Snapshot'Last = -2 and then Snapshot (-2).Number = 5);
        Check (Item_Pointers.Virtual_Length (Start) = 2 and then Start + 1 = Source (-2)'Access);
        Item_Pointers.Copy_Array (Start, Start + 1, Length => 1);
        Check (Source (-2).Number = 4 and then Snapshot (-2).Number = 5);
    end Check_Private_Element;

    procedure Check_Nulls is
        package Char_Pointers is new C.Pointers (C.size_t, C.char, C.char_array, C.nul);
        use type Char_Pointers.Pointer;
        Ref : Char_Pointers.Pointer;
        Text : C.char_array (1 .. 1) := (others => C.nul);
        Valid : constant Char_Pointers.Pointer := Text (1)'Access;
        Count : C.ptrdiff_t;
    begin
        for Path in 1 .. 7 loop
            Ref := null;
            begin
                case Path is
                    when 1 => Ref := Ref + 0;
                    when 2 => Ref := 1 + Ref;
                    when 3 => Ref := Ref - 1;
                    when 4 => Count := Valid - Ref;
                    when 5 => Count := Ref - Valid;
                    when 6 => Char_Pointers.Increment (Ref);
                    when 7 => Char_Pointers.Decrement (Ref);
                    when others => raise Program_Error;
                end case;
                raise Program_Error;
            exception
                when Char_Pointers.Pointer_Error => Check (Ref = null);
            end;
        end loop;
        for Path in 1 .. 7 loop
            begin
                case Path is
                    when 1 => Text := Char_Pointers.Value (null);
                    when 2 => Text := Char_Pointers.Value (null, Length => 0);
                    when 3 => Count := Char_Pointers.Virtual_Length (null);
                    when 4 => Char_Pointers.Copy_Array (null, Valid, Length => 0);
                    when 5 => Char_Pointers.Copy_Array (Valid, null, Length => 1);
                    when 6 => Char_Pointers.Copy_Terminated_Array (null, Valid);
                    when 7 => Char_Pointers.Copy_Terminated_Array (Valid, null, Limit => 0);
                    when others => raise Program_Error;
                end case;
                raise Program_Error;
            exception
                when C.Strings.Dereference_Error => null;
            end;
        end loop;
    end Check_Nulls;
begin
    Check_Characters;
    Check_Modular_Indices;
    Check_Private_Element;
    Check_Nulls;
    Ada.Text_IO.Put_Line ("Interfaces.C.Pointers passed");
end InterfacesCPointers;
