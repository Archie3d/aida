-- Ada 2005 container implementation; ownership and cursor machinery is shared.
package body Ada.Containers.Vectors is
    procedure Read_Container (Stream : access Ada.Streams.Root_Stream_Type'Class;
                              Item : out Vector) is
        Temporary : Vector;
        Count : Count_Type;
    begin
        Storage.Check_Cursors (Item.Data);
        Storage.Check_Elements (Item.Data);
        Storage.Ensure (Item.Data);
        Storage.Lock (Item.Data, True);
        begin
            Count := Count_Type'Input (Stream);
            for I in 1 .. Count loop
                declare
                    Element : Element_Type := Element_Type'Input (Stream);
                begin
                    Append (Temporary, Element);
                end;
            end loop;
        exception
            when others => Storage.Unlock (Item.Data, True); raise;
        end;
        Storage.Unlock (Item.Data, True);
        Move (Item, Temporary);
    end Read_Container;

    procedure Write_Container (Stream : access Ada.Streams.Root_Stream_Type'Class;
                               Item : Vector) is
        Position : Cursor := First (Item);
    begin
        Storage.Lock (Item.Data, True);
        begin
            Count_Type'Write (Stream, Length (Item));
            while Has_Element (Position) loop
                Element_Type'Output (Stream, Element (Position));
                Next (Position);
            end loop;
        exception
            when others => Storage.Unlock (Item.Data, True); raise;
        end;
        Storage.Unlock (Item.Data, True);
    end Write_Container;

    use type Storage.Position;
    use type Storage.Element_Access;

    function Box_Key (Value : Natural) return Key_Box is
    begin
        return Value;
    end Box_Key;

    function Box_Element (Value : Element_Type) return Element_Box is
    begin
        return (Value => Value);
    end Box_Element;

    function Wrap (Item : Storage.Position) return Cursor is
    begin
        return (Ptr => Item);
    end Wrap;

    function Length (Container : Vector) return Count_Type is
    begin
        return Storage.Length (Container.Data);
    end Length;

    function Is_Empty (Container : Vector) return Boolean is
    begin
        return Length (Container) = 0;
    end Is_Empty;

    procedure Clear (Container : in out Vector) is
    begin
        Storage.Clear (Container.Data);
    end Clear;

    function First (Container : Vector) return Cursor is
    begin
        return Wrap (Storage.First (Container.Data));
    end First;

    function Last (Container : Vector) return Cursor is
    begin
        return Wrap (Storage.Last (Container.Data));
    end Last;

    function Has_Element (Position : Cursor) return Boolean is
    begin
        return Position.Ptr /= null;
    end Has_Element;

    function Next (Position : Cursor) return Cursor is
    begin
        if Position.Ptr = null then return No_Element; end if;
        return Wrap (Position.Ptr.Next);
    end Next;

    procedure Next (Position : in out Cursor) is
    begin
        Position := Next (Position);
    end Next;

    function Previous (Position : Cursor) return Cursor is
    begin
        if Position.Ptr = null then return No_Element; end if;
        return Wrap (Position.Ptr.Prev);
    end Previous;

    procedure Previous (Position : in out Cursor) is
    begin
        Position := Previous (Position);
    end Previous;

    function Element (Position : Cursor) return Element_Type is
    begin
        if Position.Ptr = null or else Position.Ptr.Item = null then raise Constraint_Error; end if;
        return Position.Ptr.Item.Value;
    end Element;

    function First_Element (Container : Vector) return Element_Type is
    begin
        return Element (First (Container));
    end First_Element;

    function Last_Element (Container : Vector) return Element_Type is
    begin
        return Element (Last (Container));
    end Last_Element;

    procedure Replace_Element (Container : in out Vector; Position : Cursor; New_Item : Element_Type) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Check_Owner (Container.Data, Anchor);
        Storage.Replace (Anchor, New_Item);
    end Replace_Element;

    procedure Move (Target : in out Vector; Source : in out Vector) is
    begin
        Storage.Move (Target.Data, Source.Data);
    end Move;

    function "=" (Left, Right : Vector) return Boolean is
        L : Cursor := First (Left);
        R : Cursor := First (Right);
    begin
        if Length (Left) /= Length (Right) then return False; end if;
        while Has_Element (L) loop
            if not (Element (L) = Element (R)) then return False; end if;
            Next (L);
            Next (R);
        end loop;
        return True;
    end "=";

    procedure Query_Element (Position : Cursor; Process : not null access procedure (Element : in Element_Type)) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Lock (Anchor, True);
        begin
            Process (Anchor.Item.Value);
        exception
            when others => Storage.Unlock (Anchor, True); raise;
        end;
        Storage.Unlock (Anchor, True);
    end Query_Element;

    procedure Update_Element (Container : in out Vector; Position : Cursor; Process : not null access procedure (Element : in out Element_Type)) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Check_Owner (Container.Data, Anchor);
        Storage.Check_Elements (Container.Data);
        Storage.Lock (Anchor, True);
        begin
            Process (Anchor.Item.Value);
        exception
            when others => Storage.Unlock (Anchor, True); raise;
        end;
        Storage.Unlock (Anchor, True);
    end Update_Element;

    procedure Iterate (Container : Vector; Process : not null access procedure (Position : Cursor)) is
        P : Cursor := First (Container);
    begin
        Storage.Lock (Container.Data);
        begin
            while Has_Element (P) loop
                Process (P);
                Next (P);
            end loop;
        exception
            when others => Storage.Unlock (Container.Data); raise;
        end;
        Storage.Unlock (Container.Data);
    end Iterate;

    procedure Reverse_Iterate (Container : Vector; Process : not null access procedure (Position : Cursor)) is
        P : Cursor := Last (Container);
    begin
        Storage.Lock (Container.Data);
        begin
            while Has_Element (P) loop
                Process (P);
                Previous (P);
            end loop;
        exception
            when others => Storage.Unlock (Container.Data); raise;
        end;
        Storage.Unlock (Container.Data);
    end Reverse_Iterate;

    function Find (Container : Vector; Item : Element_Type; Position : Cursor := No_Element) return Cursor is
        P : Cursor := Position;
    begin
        Storage.Check_Owner (Container.Data, Position.Ptr, True);
        if not Has_Element (P) then P := First (Container); end if;
        while Has_Element (P) loop
            if Element (P) = Item then return P; end if;
            Next (P);
        end loop;
        return No_Element;
    end Find;

    function Reverse_Find (Container : Vector; Item : Element_Type; Position : Cursor := No_Element) return Cursor is
        P : Cursor := Position;
    begin
        Storage.Check_Owner (Container.Data, Position.Ptr, True);
        if not Has_Element (P) then P := Last (Container); end if;
        while Has_Element (P) loop
            if Element (P) = Item then return P; end if;
            Previous (P);
        end loop;
        return No_Element;
    end Reverse_Find;

    function Contains (Container : Vector; Item : Element_Type) return Boolean is
    begin
        return Has_Element (Find (Container, Item));
    end Contains;

    function Max_Length return Count_Type is
        F : Long_Integer := Long_Integer (Index_Type'First);
        L : Long_Integer := Long_Integer (Index_Type'Last);
    begin
        if L < F then return 0; end if;
        if L >= Long_Integer'First + Long_Integer (Count_Type'Last) then
            if F <= L - Long_Integer (Count_Type'Last) then return Count_Type'Last; end if;
        end if;
        return Count_Type (L - F + 1);
    end Max_Length;

    procedure Check_Length (Size : Count_Type) is
    begin
        if Size > Max_Length then raise Constraint_Error; end if;
    end Check_Length;

    procedure Grow (Container : in out Vector; Needed : Count_Type) is
        Size : Count_Type := Storage.Capacity (Container.Data);
    begin
        Check_Length (Needed);
        if Needed <= Size then return; end if;
        if Size <= Max_Length / 2 then Size := Size * 2; else Size := Max_Length; end if;
        if Size < Needed then Size := Needed; end if;
        Storage.Reserve (Container.Data, Size);
    end Grow;

    function Slot (Container : Vector; Index : Extended_Index; End_Allowed : Boolean := False) return Count_Type is
        Offset : Long_Integer;
    begin
        if Index < Index_Type'First then raise Constraint_Error; end if;
        Offset := Long_Integer (Index) - Long_Integer (Index_Type'First);
        if Offset > Long_Integer (Length (Container)) or else
            (Offset = Long_Integer (Length (Container)) and not End_Allowed) then
            raise Constraint_Error;
        end if;
        return Count_Type (Offset + 1);
    end Slot;

    function At_Index (Container : Vector; Index : Extended_Index) return Cursor is
    begin
        return Wrap (Storage.At_Index (Container.Data, Slot (Container, Index)));
    end At_Index;

    function Before_Cursor (Container : Vector; Index : Extended_Index) return Cursor is
        I : Count_Type;
    begin
        I := Slot (Container, Index, True);
        if I > Length (Container) then return No_Element; end if;
        return Wrap (Storage.At_Index (Container.Data, I));
    end Before_Cursor;

    function Before_Index (Container : Vector; Before : Cursor) return Extended_Index is
    begin
        Storage.Check_Owner (Container.Data, Before.Ptr, True);
        if Before.Ptr = null then
            return Extended_Index (Long_Integer (Index_Type'First) + Long_Integer (Length (Container)));
        end if;
        return To_Index (Before);
    end Before_Index;

    function To_Cursor (Container : Vector; Index : Extended_Index) return Cursor is
    begin
        if Index < Index_Type'First or else Index > Last_Index (Container) then return No_Element; end if;
        return At_Index (Container, Index);
    end To_Cursor;

    function To_Index (Position : Cursor) return Extended_Index is
    begin
        if Position.Ptr = null then return No_Index; end if;
        return Extended_Index (Long_Integer (Index_Type'First) + Long_Integer (Position.Ptr.Index - 1));
    end To_Index;

    function First_Index (Container : Vector) return Index_Type is
    begin
        return Index_Type'First;
    end First_Index;

    function Last_Index (Container : Vector) return Extended_Index is
    begin
        if Length (Container) = 0 then return No_Index; end if;
        return To_Index (Last (Container));
    end Last_Index;

    function Capacity (Container : Vector) return Count_Type is
    begin
        return Storage.Capacity (Container.Data);
    end Capacity;

    procedure Reserve_Capacity (Container : in out Vector; Capacity : Count_Type) is
    begin
        Check_Length (Capacity);
        Storage.Reserve (Container.Data, Capacity);
    end Reserve_Capacity;

    function Element (Container : Vector; Index : Index_Type) return Element_Type is
    begin
        return Element (At_Index (Container, Index));
    end Element;

    procedure Replace_Element (Container : in out Vector; Index : Index_Type; New_Item : Element_Type) is
    begin
        Replace_Element (Container, At_Index (Container, Index), New_Item);
    end Replace_Element;

    procedure Query_Element (Container : Vector; Index : Index_Type; Process : not null access procedure (Element : in Element_Type)) is
    begin
        Query_Element (At_Index (Container, Index), Process);
    end Query_Element;

    procedure Update_Element (Container : in out Vector; Index : Index_Type; Process : not null access procedure (Element : in out Element_Type)) is
    begin
        Update_Element (Container, At_Index (Container, Index), Process);
    end Update_Element;

    procedure Check_Growth (Container : Vector; Count : Count_Type) is
    begin
        if Count > Max_Length - Length (Container) then raise Constraint_Error; end if;
    end Check_Growth;

    procedure Validate_Elements (Container : Vector) is
        P : Storage.Position := Storage.First (Container.Data);
    begin
        while P /= null loop
            if P.Item = null then raise Constraint_Error with "empty vector element"; end if;
            P := P.Next;
        end loop;
    end Validate_Elements;

    procedure Insert (Container : in out Vector; Before : Cursor; New_Item : Element_Type; Position : out Cursor; Count : Count_Type := 1) is
        Anchor : Storage.Position := Before.Ptr;
        Pending : Storage.Store;
        P : Storage.Position;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Anchor, True);
        Check_Growth (Container, Count);
        if Count = 0 then Position := Before; return; end if;
        for I in 1 .. Count loop
            P := Storage.Create (New_Item);
            begin Storage.Link (Pending, null, P);
            exception when others => Storage.Destroy (P); raise;
            end;
        end loop;
        Grow (Container, Length (Container) + Count);
        Position := Wrap (Storage.First (Pending));
        while Storage.First (Pending) /= null loop
            P := Storage.First (Pending);
            Storage.Unlink (Pending, P);
            Storage.Link (Container.Data, Anchor, P);
        end loop;
    end Insert;

    procedure Insert (Container : in out Vector; Before : Cursor; New_Item : Element_Type; Count : Count_Type := 1) is
        P : Cursor;
    begin
        Insert (Container, Before, New_Item, P, Count);
    end Insert;

    procedure Append (Container : in out Vector; New_Item : Element_Type; Count : Count_Type := 1) is
    begin
        Insert (Container, No_Element, New_Item, Count);
    end Append;

    procedure Prepend (Container : in out Vector; New_Item : Element_Type; Count : Count_Type := 1) is
    begin
        Insert (Container, First (Container), New_Item, Count);
    end Prepend;

    procedure Insert (Container : in out Vector; Before : Cursor; Position : out Cursor; Count : Count_Type := 1) is
        Anchor : Storage.Position := Before.Ptr;
        Pending : Storage.Store;
        P : Storage.Position;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Anchor, True);
        Check_Growth (Container, Count);
        if Count = 0 then Position := Before; return; end if;
        for I in 1 .. Count loop
            P := new Storage.Node;
            begin
                P.Item := new Element_Box;
                Storage.Link (Pending, null, P);
            exception when others => Storage.Destroy (P); raise;
            end;
        end loop;
        Grow (Container, Length (Container) + Count);
        Position := Wrap (Storage.First (Pending));
        while Storage.First (Pending) /= null loop
            P := Storage.First (Pending);
            Storage.Unlink (Pending, P);
            Storage.Link (Container.Data, Anchor, P);
        end loop;
    end Insert;

    procedure Delete (Container : in out Vector; Position : in out Cursor; Count : Count_Type := 1) is
        P, N : Storage.Position;
        Remaining : Count_Type := Count;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Position.Ptr);
        P := Position.Ptr;
        Position := No_Element;
        while P /= null and Remaining > 0 loop
            N := P.Next;
            Storage.Delete (Container.Data, P);
            P := N;
            Remaining := Remaining - 1;
        end loop;
    end Delete;

    procedure Delete_First (Container : in out Vector; Count : Count_Type := 1) is
        P : Cursor := First (Container);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P, Count); end if;
    end Delete_First;

    procedure Delete_Last (Container : in out Vector; Count : Count_Type := 1) is
        P : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        for I in 1 .. Count loop
            P := Last (Container);
            exit when not Has_Element (P);
            Delete (Container, P);
        end loop;
    end Delete_Last;

    procedure Swap (Container : in out Vector; I, J : Cursor) is
    begin
        Storage.Check_Owner (Container.Data, I.Ptr);
        Storage.Check_Owner (Container.Data, J.Ptr);
        Storage.Swap_Items (I.Ptr, J.Ptr);
    end Swap;

    procedure Swap (Container : in out Vector; I, J : Index_Type) is
    begin
        Swap (Container, At_Index (Container, I), At_Index (Container, J));
    end Swap;

    procedure Reverse_Elements (Container : in out Vector) is
    begin
        Storage.Check_Elements (Container.Data);
        Validate_Elements (Container);
        for I in 1 .. Length (Container) / 2 loop
            Storage.Swap_Items (Storage.At_Index (Container.Data, I),
                Storage.At_Index (Container.Data, Length (Container) - I + 1));
        end loop;
    end Reverse_Elements;

    procedure Insert (Container : in out Vector; Before : Extended_Index; New_Item : Element_Type; Count : Count_Type := 1) is
    begin
        Insert (Container, Before_Cursor (Container, Before), New_Item, Count);
    end Insert;

    procedure Insert (Container : in out Vector; Before : Cursor; New_Item : Vector; Position : out Cursor) is
        Anchor : Storage.Position := Before.Ptr;
        Pending : Vector := New_Item;
        P : Cursor := First (Pending);
        N : Storage.Position;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Anchor, True);
        Check_Growth (Container, Length (Pending));
        if Length (Pending) = 0 then Position := Before; return; end if;
        Grow (Container, Length (Container) + Length (Pending));
        Position := First (Pending);
        while Has_Element (P) loop
            N := P.Ptr;
            Next (P);
            Storage.Unlink (Pending.Data, N);
            Storage.Link (Container.Data, Anchor, N);
        end loop;
    end Insert;

    procedure Insert (Container : in out Vector; Before : Cursor; New_Item : Vector) is
        P : Cursor;
    begin
        Insert (Container, Before, New_Item, P);
    end Insert;

    procedure Insert (Container : in out Vector; Before : Extended_Index; New_Item : Vector) is
    begin
        Insert (Container, Before_Cursor (Container, Before), New_Item);
    end Insert;

    procedure Append (Container : in out Vector; New_Item : Vector) is
    begin
        Insert (Container, No_Element, New_Item);
    end Append;

    procedure Prepend (Container : in out Vector; New_Item : Vector) is
    begin
        Insert (Container, First (Container), New_Item);
    end Prepend;

    procedure Insert_Space (Container : in out Vector; Before : Cursor; Position : out Cursor; Count : Count_Type := 1) is
        Anchor : Storage.Position := Before.Ptr;
        P : Storage.Position;
        Pending : Storage.Store;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Anchor, True);
        Check_Growth (Container, Count);
        if Count = 0 then Position := Before; return; end if;
        for I in 1 .. Count loop
            P := new Storage.Node;
            begin Storage.Link (Pending, null, P);
            exception when others => Storage.Destroy (P); raise;
            end;
        end loop;
        Grow (Container, Length (Container) + Count);
        Position := Wrap (Storage.First (Pending));
        while Storage.First (Pending) /= null loop
            P := Storage.First (Pending);
            Storage.Unlink (Pending, P);
            Storage.Link (Container.Data, Anchor, P);
        end loop;
    end Insert_Space;

    procedure Insert_Space (Container : in out Vector; Before : Extended_Index; Count : Count_Type := 1) is
        P : Cursor;
    begin
        Insert_Space (Container, Before_Cursor (Container, Before), P, Count);
    end Insert_Space;

    procedure Insert (Container : in out Vector; Before : Extended_Index; Count : Count_Type := 1) is
        P : Cursor;
    begin
        Insert (Container, Before_Cursor (Container, Before), P, Count);
    end Insert;

    procedure Set_Length (Container : in out Vector; Length : Count_Type) is
        P : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        Check_Length (Length);
        if Length <= Storage.Length (Container.Data) then
            Delete_Last (Container, Storage.Length (Container.Data) - Length);
        else
            Insert (Container, No_Element, P, Length - Storage.Length (Container.Data));
        end if;
    end Set_Length;

    function To_Vector (Length : Count_Type) return Vector is
        Result : Vector;
        P : Cursor;
    begin
        Insert (Result, No_Element, P, Length);
        return Result;
    end To_Vector;

    function To_Vector (New_Item : Element_Type; Length : Count_Type) return Vector is
        Result : Vector;
    begin
        Append (Result, New_Item, Length);
        return Result;
    end To_Vector;

    function "&" (Left : Vector; Right : Vector) return Vector is
        Result : Vector;
    begin
        Append (Result, Left);
        Append (Result, Right);
        return Result;
    end "&";

    function "&" (Left : Vector; Right : Element_Type) return Vector is
        Result : Vector;
    begin
        Append (Result, Left);
        Append (Result, Right);
        return Result;
    end "&";

    function "&" (Left : Element_Type; Right : Vector) return Vector is
        Result : Vector;
    begin
        Append (Result, Left);
        Append (Result, Right);
        return Result;
    end "&";

    function "&" (Left : Element_Type; Right : Element_Type) return Vector is
        Result : Vector;
    begin
        Append (Result, Left);
        Append (Result, Right);
        return Result;
    end "&";

    procedure Delete (Container : in out Vector; Index : Extended_Index; Count : Count_Type := 1) is
        P : Cursor := Before_Cursor (Container, Index);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P, Count); end if;
    end Delete;

    function Find_Index (Container : Vector; Item : Element_Type; Index : Index_Type := Index_Type'First) return Extended_Index is
        P : Cursor := First (Container);
    begin
        while Has_Element (P) loop
            if To_Index (P) >= Index and then Element (P) = Item then
                return To_Index (P);
            end if;
            Next (P);
        end loop;
        return No_Index;
    end Find_Index;

    function Reverse_Find_Index (Container : Vector; Item : Element_Type; Index : Index_Type := Index_Type'Last) return Extended_Index is
        P : Cursor := Last (Container);
    begin
        while Has_Element (P) loop
            if To_Index (P) <= Index and then Element (P) = Item then
                return To_Index (P);
            end if;
            Previous (P);
        end loop;
        return No_Index;
    end Reverse_Find_Index;

    package body Generic_Sorting is
        function Is_Sorted (Container : Vector) return Boolean is
            P : Cursor := First (Container);
            Q : Cursor;
        begin
            if not Has_Element (P) then return True; end if;
            Q := Next (P);
            while Has_Element (Q) loop
                if Element (Q) < Element (P) then return False; end if;
                P := Q;
                Next (Q);
            end loop;
            return True;
        end Is_Sorted;

        procedure Sort (Container : in out Vector) is
            procedure Raw_Swap (I, J : Count_Type) is
                P : Storage.Position := Storage.At_Index (Container.Data, I);
                Q : Storage.Position := Storage.At_Index (Container.Data, J);
                Temp : Storage.Element_Access := P.Item;
            begin
                P.Item := Q.Item;
                Q.Item := Temp;
            end Raw_Swap;
            procedure Sift (Start, Finish : Count_Type) is
                Root : Count_Type := Start;
                Child : Count_Type;
            begin
                while Root <= Finish / 2 loop
                    Child := Root * 2;
                    if Child < Finish and then Element (Wrap (Storage.At_Index (Container.Data, Child))) <
                        Element (Wrap (Storage.At_Index (Container.Data, Child + 1))) then Child := Child + 1; end if;
                    exit when not (Element (Wrap (Storage.At_Index (Container.Data, Root))) <
                        Element (Wrap (Storage.At_Index (Container.Data, Child))));
                    Raw_Swap (Root, Child);
                    Root := Child;
                end loop;
            end Sift;
        begin
            Storage.Check_Elements (Container.Data);
            Validate_Elements (Container);
            Storage.Lock (Container.Data, True);
            begin
                for I in reverse 1 .. Length (Container) / 2 loop
                    Sift (I, Length (Container));
                end loop;
                for I in reverse 2 .. Length (Container) loop
                    Raw_Swap (1, I);
                    Sift (1, I - 1);
                end loop;
            exception
                when others => Storage.Unlock (Container.Data, True); raise;
            end;
            Storage.Unlock (Container.Data, True);
        end Sort;

        procedure Merge (Target : in out Vector; Source : in out Vector) is
            P : Cursor := First (Target);
            Q : Cursor := First (Source);
            N : Cursor;
        begin
            Storage.Check_Cursors (Target.Data);
            Storage.Check_Cursors (Source.Data);
            Storage.Check_Elements (Target.Data);
            Storage.Check_Elements (Source.Data);
            if Target'Address = Source'Address then return; end if;
            Validate_Elements (Target);
            Validate_Elements (Source);
            Check_Growth (Target, Length (Source));
            Grow (Target, Length (Target) + Length (Source));
            Storage.Lock (Target.Data);
            Storage.Lock (Source.Data);
            begin
                while Has_Element (Q) loop
                    while Has_Element (P) and then not (Element (Q) < Element (P)) loop Next (P); end loop;
                    N := Next (Q);
                    Storage.Unlink (Source.Data, Q.Ptr);
                    Storage.Link (Target.Data, P.Ptr, Q.Ptr);
                    Q := N;
                end loop;
            exception
                when others => Storage.Unlock (Source.Data); Storage.Unlock (Target.Data); raise;
            end;
            Storage.Unlock (Source.Data);
            Storage.Unlock (Target.Data);
        end Merge;

    end Generic_Sorting;
end Ada.Containers.Vectors;
