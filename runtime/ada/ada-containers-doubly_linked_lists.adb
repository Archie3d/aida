-- Ada 2005 container implementation; ownership and cursor machinery is shared.
package body Ada.Containers.Doubly_Linked_Lists is
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

    function Length (Container : List) return Count_Type is
    begin
        return Storage.Length (Container.Data);
    end Length;

    function Is_Empty (Container : List) return Boolean is
    begin
        return Length (Container) = 0;
    end Is_Empty;

    procedure Clear (Container : in out List) is
    begin
        Storage.Clear (Container.Data);
    end Clear;

    function First (Container : List) return Cursor is
    begin
        return Wrap (Storage.First (Container.Data));
    end First;

    function Last (Container : List) return Cursor is
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

    function First_Element (Container : List) return Element_Type is
    begin
        return Element (First (Container));
    end First_Element;

    function Last_Element (Container : List) return Element_Type is
    begin
        return Element (Last (Container));
    end Last_Element;

    procedure Replace_Element (Container : in out List; Position : Cursor; New_Item : Element_Type) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Check_Owner (Container.Data, Anchor);
        Storage.Replace (Anchor, New_Item);
    end Replace_Element;

    procedure Move (Target : in out List; Source : in out List) is
    begin
        Storage.Move (Target.Data, Source.Data);
    end Move;

    function "=" (Left, Right : List) return Boolean is
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

    procedure Update_Element (Container : in out List; Position : Cursor; Process : not null access procedure (Element : in out Element_Type)) is
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

    procedure Iterate (Container : List; Process : not null access procedure (Position : Cursor)) is
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

    procedure Reverse_Iterate (Container : List; Process : not null access procedure (Position : Cursor)) is
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

    function Find (Container : List; Item : Element_Type; Position : Cursor := No_Element) return Cursor is
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

    function Reverse_Find (Container : List; Item : Element_Type; Position : Cursor := No_Element) return Cursor is
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

    function Contains (Container : List; Item : Element_Type) return Boolean is
    begin
        return Has_Element (Find (Container, Item));
    end Contains;

    procedure Insert (Container : in out List; Before : Cursor; New_Item : Element_Type; Position : out Cursor; Count : Count_Type := 1) is
        Anchor : Storage.Position := Before.Ptr;
        Pending : Storage.Store;
        P : Storage.Position;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Anchor, True);
        if Count > Count_Type'Last - Length (Container) then raise Constraint_Error; end if;
        for I in 1 .. Count loop
            P := Storage.Create (New_Item);
            begin Storage.Link (Pending, null, P);
            exception when others => Storage.Destroy (P); raise;
            end;
        end loop;

        Position := Wrap (Storage.First (Pending));
        while Storage.First (Pending) /= null loop
            P := Storage.First (Pending);
            Storage.Unlink (Pending, P);
            Storage.Link (Container.Data, Anchor, P);
        end loop;
    end Insert;

    procedure Insert (Container : in out List; Before : Cursor; New_Item : Element_Type; Count : Count_Type := 1) is
        P : Cursor;
    begin
        Insert (Container, Before, New_Item, P, Count);
    end Insert;

    procedure Append (Container : in out List; New_Item : Element_Type; Count : Count_Type := 1) is
    begin
        Insert (Container, No_Element, New_Item, Count);
    end Append;

    procedure Prepend (Container : in out List; New_Item : Element_Type; Count : Count_Type := 1) is
    begin
        Insert (Container, First (Container), New_Item, Count);
    end Prepend;

    procedure Insert (Container : in out List; Before : Cursor; Position : out Cursor; Count : Count_Type := 1) is
        Anchor : Storage.Position := Before.Ptr;
        Pending : Storage.Store;
        P : Storage.Position;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Anchor, True);

        for I in 1 .. Count loop
            P := new Storage.Node;
            begin
                P.Item := new Element_Box;
                Storage.Link (Pending, null, P);
            exception when others => Storage.Destroy (P); raise;
            end;
        end loop;

        Position := Wrap (Storage.First (Pending));
        while Storage.First (Pending) /= null loop
            P := Storage.First (Pending);
            Storage.Unlink (Pending, P);
            Storage.Link (Container.Data, Anchor, P);
        end loop;
    end Insert;

    procedure Delete (Container : in out List; Position : in out Cursor; Count : Count_Type := 1) is
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

    procedure Delete_First (Container : in out List; Count : Count_Type := 1) is
        P : Cursor := First (Container);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P, Count); end if;
    end Delete_First;

    procedure Delete_Last (Container : in out List; Count : Count_Type := 1) is
        P : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        for I in 1 .. Count loop
            P := Last (Container);
            exit when not Has_Element (P);
            Delete (Container, P);
        end loop;
    end Delete_Last;

    procedure Swap (Container : in out List; I, J : Cursor) is
    begin
        Storage.Check_Owner (Container.Data, I.Ptr);
        Storage.Check_Owner (Container.Data, J.Ptr);
        Storage.Swap_Items (I.Ptr, J.Ptr);
    end Swap;

    procedure Reverse_Elements (Container : in out List) is
    begin
        Storage.Check_Cursors (Container.Data);
        if not Is_Empty (Container) then Storage.Reverse_Links (Container.Data); end if;
    end Reverse_Elements;

    procedure Swap_Links (Container : in out List; I, J : Cursor) is
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, I.Ptr);
        Storage.Check_Owner (Container.Data, J.Ptr);
        Storage.Swap_Links (Container.Data, I.Ptr, J.Ptr);
    end Swap_Links;

    procedure Splice (Target : in out List; Before : Cursor; Source : in out List; Position : in out Cursor) is
    begin
        Storage.Check_Cursors (Target.Data);
        Storage.Check_Cursors (Source.Data);
        Storage.Check_Owner (Target.Data, Before.Ptr, True);
        Storage.Check_Owner (Source.Data, Position.Ptr);
        if Target'Address /= Source'Address and then Length (Target) = Count_Type'Last then raise Constraint_Error; end if;
        if Target'Address = Source'Address then
            if Before = Position or else Before = Next (Position) then return; end if;
        end if;
        Storage.Ensure (Target.Data);
        Storage.Unlink (Source.Data, Position.Ptr);
        Storage.Link (Target.Data, Before.Ptr, Position.Ptr);
    end Splice;

    procedure Splice (Target : in out List; Before : Cursor; Source : in out List) is
        P : Cursor;
    begin
        Storage.Check_Cursors (Target.Data);
        Storage.Check_Cursors (Source.Data);
        Storage.Check_Owner (Target.Data, Before.Ptr, True);
        if Target'Address = Source'Address then return; end if;
        if Length (Source) > Count_Type'Last - Length (Target) then raise Constraint_Error; end if;
        Storage.Ensure (Target.Data);
        while not Is_Empty (Source) loop
            P := First (Source);
            Splice (Target, Before, Source, P);
        end loop;
    end Splice;

    procedure Splice (Container : in out List; Before : Cursor; Position : Cursor) is
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Owner (Container.Data, Before.Ptr, True);
        Storage.Check_Owner (Container.Data, Position.Ptr);
        if Before = Position or else Before = Next (Position) then return; end if;
        Storage.Unlink (Container.Data, Position.Ptr);
        Storage.Link (Container.Data, Before.Ptr, Position.Ptr);
    end Splice;

    package body Generic_Sorting is
        function Is_Sorted (Container : List) return Boolean is
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

        procedure Sort (Container : in out List) is
            P, Q, N, Start, After_Run : Cursor;
            Width : Count_Type := 1;
            Left_Count, Right_Count : Count_Type;
        begin
            Storage.Check_Cursors (Container.Data);
            Storage.Check_Elements (Container.Data);
            Storage.Lock (Container.Data, True);
            begin
                while Width < Length (Container) loop
                    Start := First (Container);
                    while Has_Element (Start) loop
                        P := Start;
                        Q := Start;
                        for I in 1 .. Width loop
                            exit when not Has_Element (Q);
                            Next (Q);
                        end loop;
                        exit when not Has_Element (Q);
                        After_Run := Q;
                        for I in 1 .. Width loop
                            exit when not Has_Element (After_Run);
                            Next (After_Run);
                        end loop;
                        Left_Count := Width;
                        Right_Count := Width;
                        while Left_Count > 0 and Right_Count > 0 and Has_Element (Q) loop
                            if Element (Q) < Element (P) then
                                N := Next (Q);
                                Storage.Unlink (Container.Data, Q.Ptr);
                                Storage.Link (Container.Data, P.Ptr, Q.Ptr);
                                Q := N;
                                Right_Count := Right_Count - 1;
                            else
                                Next (P);
                                Left_Count := Left_Count - 1;
                            end if;
                        end loop;
                        Start := After_Run;
                    end loop;
                    exit when Width > Length (Container) / 2;
                    Width := Width * 2;
                end loop;
            exception
                when others => Storage.Unlock (Container.Data, True); raise;
            end;
            Storage.Unlock (Container.Data, True);
        end Sort;

        procedure Merge (Target : in out List; Source : in out List) is
            P : Cursor := First (Target);
            Q : Cursor := First (Source);
            N : Cursor;
        begin
            Storage.Check_Cursors (Target.Data);
            Storage.Check_Cursors (Source.Data);
            Storage.Check_Elements (Target.Data);
            Storage.Check_Elements (Source.Data);
            if Target'Address = Source'Address then return; end if;
            if Length (Source) > Count_Type'Last - Length (Target) then raise Constraint_Error; end if;
            Storage.Ensure (Target.Data);
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
end Ada.Containers.Doubly_Linked_Lists;
