-- Ada 2005 container implementation; ownership and cursor machinery is shared.
package body Ada.Containers.Indefinite_Ordered_Maps is
    use type Storage.Position;
    use type Storage.Element_Access;

    function Box_Key (Value : Key_Type) return Key_Box is
    begin
        return Value;
    end Box_Key;

    function Box_Element (Value : Element_Type) return Element_Box is
    begin
        return Value;
    end Box_Element;

    function Wrap (Item : Storage.Position) return Cursor is
    begin
        return (Ptr => Item);
    end Wrap;

    function Length (Container : Map) return Count_Type is
    begin
        return Storage.Length (Container.Data);
    end Length;

    function Is_Empty (Container : Map) return Boolean is
    begin
        return Length (Container) = 0;
    end Is_Empty;

    procedure Clear (Container : in out Map) is
    begin
        Storage.Clear (Container.Data);
    end Clear;

    procedure Move (Target : in out Map; Source : in out Map) is
    begin
        Storage.Move (Target.Data, Source.Data);
    end Move;

    function First (Container : Map) return Cursor is
    begin
        return Wrap (Storage.First (Container.Data));
    end First;

    function Next (Position : Cursor) return Cursor is
    begin
        if Position.Ptr = null then return No_Element; end if;
        return Wrap (Position.Ptr.Next);
    end Next;

    procedure Next (Position : in out Cursor) is
    begin
        Position := Next (Position);
    end Next;

    function Has_Element (Position : Cursor) return Boolean is
    begin
        return Position.Ptr /= null;
    end Has_Element;

    function Element (Position : Cursor) return Element_Type is
    begin
        if Position.Ptr = null then raise Constraint_Error; end if;
        return Position.Ptr.Item.all;
    end Element;

    function Equivalent_Keys (Left, Right : Key_Type) return Boolean is
    begin
        return not (Left < Right) and not (Right < Left);
    end Equivalent_Keys;

    function Key (Position : Cursor) return Key_Type is
    begin
        if Position.Ptr = null then raise Constraint_Error; end if;
        return Position.Ptr.Key.all;
    end Key;

    function Stored_Key (Position : Cursor) return Key_Type is
    begin
        return Key (Position);
    end Stored_Key;

    function Find (Container : Map; Key : Key_Type) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
    begin
        Storage.Lock (Container.Data, True);
        begin
            while Has_Element (P) loop
                if Equivalent_Keys (Stored_Key (P), Key) then exit; end if;
                if Key < Stored_Key (P) then P := Wrap (P.Ptr.Left);
                else P := Wrap (P.Ptr.Right);
                end if;
            end loop;
        exception
            when others => Storage.Unlock (Container.Data, True); raise;
        end;
        Storage.Unlock (Container.Data, True);
        return P;
    end Find;

    function Contains (Container : Map; Key : Key_Type) return Boolean is
    begin
        return Has_Element (Find (Container, Key));
    end Contains;

    function Element (Container : Map; Key : Key_Type) return Element_Type is
    begin
        return Element (Find (Container, Key));
    end Element;

    function Insertion_Point (Container : Map; Item : Key_Type; Skip : Cursor := No_Element) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
        Candidate : Cursor;
    begin
        while Has_Element (P) loop
            if Item < Stored_Key (P) then
                Candidate := P;
                P := Wrap (P.Ptr.Left);
            else
                P := Wrap (P.Ptr.Right);
            end if;
        end loop;
        if Candidate = Skip then return Next (Candidate); end if;
        return Candidate;
    end Insertion_Point;

    procedure Iterate (Container : Map; Process : not null access procedure (Position : Cursor)) is
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

    procedure Reverse_Iterate (Container : Map; Process : not null access procedure (Position : Cursor)) is
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

    procedure Query_Element (Position : Cursor; Process : not null access procedure (Key : Key_Type; Element : Element_Type)) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Lock (Anchor, True);
        begin
            Process (Anchor.Key.all, Anchor.Item.all);
        exception
            when others => Storage.Unlock (Anchor, True); raise;
        end;
        Storage.Unlock (Anchor, True);
    end Query_Element;

    procedure Update_Element (Container : in out Map; Position : Cursor; Process : not null access procedure (Key : Key_Type; Element : in out Element_Type)) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Check_Owner (Container.Data, Anchor);
        Storage.Check_Elements (Container.Data);
        Storage.Lock (Anchor, True);
        begin
            Process (Anchor.Key.all, Anchor.Item.all);
        exception
            when others => Storage.Unlock (Anchor, True); raise;
        end;
        Storage.Unlock (Anchor, True);
    end Update_Element;

    procedure Replace_Element (Container : in out Map; Position : Cursor; New_Item : Element_Type) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Check_Owner (Container.Data, Anchor);
        Storage.Replace (Anchor, New_Item);
    end Replace_Element;

    procedure Insert (Container : in out Map; Key : Key_Type; New_Item : Element_Type; Position : out Cursor; Inserted : out Boolean) is
        Pending : Storage.Position := null;
        Before : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        Position := Find (Container, Key);
        Inserted := not Has_Element (Position);
        if not Inserted then return; end if;
        Before := Insertion_Point (Container, Key);
        Pending := Storage.Create (New_Item);
        Storage.Set_Key (Pending, Key);

        Storage.Link (Container.Data, Before.Ptr, Pending);
        Position := Wrap (Pending);
        Pending := null;
    exception
        when others => Storage.Destroy (Pending); raise;
    end Insert;

    procedure Insert (Container : in out Map; Key : Key_Type; New_Item : Element_Type) is
        P : Cursor;
        Inserted : Boolean;
    begin
        Insert (Container, Key, New_Item, P, Inserted);
        if not Inserted then raise Constraint_Error with "duplicate container key"; end if;
    end Insert;

    procedure Include (Container : in out Map; Key : Key_Type; New_Item : Element_Type) is
        P : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        P := Find (Container, Key);
        if not Has_Element (P) then
            Insert (Container, Key, New_Item);
        else
            Storage.Set_Key (P.Ptr, Key);
            Replace_Element (Container, P, New_Item);
        end if;
    end Include;

    procedure Replace (Container : in out Map; Key : Key_Type; New_Item : Element_Type) is
        P : Cursor;
    begin
        Storage.Check_Elements (Container.Data);
        P := Find (Container, Key);
        if not Has_Element (P) then
            raise Constraint_Error with "key not found";
        else
            Storage.Set_Key (P.Ptr, Key);
            Replace_Element (Container, P, New_Item);
        end if;
    end Replace;

    procedure Delete (Container : in out Map; Position : in out Cursor) is
    begin
        Storage.Delete (Container.Data, Position.Ptr);
    end Delete;

    procedure Delete (Container : in out Map; Key : Key_Type) is
        P : Cursor := Find (Container, Key);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P);
        else raise Constraint_Error with "key not found";
        end if;
    end Delete;

    procedure Exclude (Container : in out Map; Key : Key_Type) is
        P : Cursor := Find (Container, Key);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P);

        end if;
    end Exclude;

    function Last (Container : Map) return Cursor is
    begin
        return Wrap (Storage.Last (Container.Data));
    end Last;

    function Previous (Position : Cursor) return Cursor is
    begin
        if Position.Ptr = null then return No_Element; end if;
        return Wrap (Position.Ptr.Prev);
    end Previous;

    procedure Previous (Position : in out Cursor) is
    begin
        Position := Previous (Position);
    end Previous;

    function First_Element (Container : Map) return Element_Type is
    begin
        return Element (First (Container));
    end First_Element;

    function First_Key (Container : Map) return Key_Type is
    begin
        return Key (First (Container));
    end First_Key;

    procedure Delete_First (Container : in out Map) is
        P : Cursor := First (Container);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P); end if;
    end Delete_First;

    function Last_Element (Container : Map) return Element_Type is
    begin
        return Element (Last (Container));
    end Last_Element;

    function Last_Key (Container : Map) return Key_Type is
    begin
        return Key (Last (Container));
    end Last_Key;

    procedure Delete_Last (Container : in out Map) is
        P : Cursor := Last (Container);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P); end if;
    end Delete_Last;

    function Floor (Container : Map; Key : Key_Type) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
        Candidate : Cursor;
    begin
        Storage.Lock (Container.Data, True);
        begin
            while Has_Element (P) loop
                if Key < Stored_Key (P) then
                    P := Wrap (P.Ptr.Left);
                else
                    Candidate := P;
                    P := Wrap (P.Ptr.Right);
                end if;
            end loop;
        exception
            when others => Storage.Unlock (Container.Data, True); raise;
        end;
        Storage.Unlock (Container.Data, True);
        return Candidate;
    end Floor;

    function Ceiling (Container : Map; Key : Key_Type) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
        Candidate : Cursor;
    begin
        Storage.Lock (Container.Data, True);
        begin
            while Has_Element (P) loop
                if Stored_Key (P) < Key then
                    P := Wrap (P.Ptr.Right);
                else
                    Candidate := P;
                    P := Wrap (P.Ptr.Left);
                end if;
            end loop;
        exception
            when others => Storage.Unlock (Container.Data, True); raise;
        end;
        Storage.Unlock (Container.Data, True);
        return Candidate;
    end Ceiling;

    function "<" (Left : Cursor; Right : Cursor) return Boolean is
    begin
        return Stored_Key (Left) < Stored_Key (Right);
    end "<";

    function "<" (Left : Cursor; Right : Key_Type) return Boolean is
    begin
        return Stored_Key (Left) < Right;
    end "<";

    function "<" (Left : Key_Type; Right : Cursor) return Boolean is
    begin
        return Left < Stored_Key (Right);
    end "<";

    function ">" (Left : Cursor; Right : Cursor) return Boolean is
    begin
        return Stored_Key (Right) < Stored_Key (Left);
    end ">";

    function ">" (Left : Cursor; Right : Key_Type) return Boolean is
    begin
        return Right < Stored_Key (Left);
    end ">";

    function ">" (Left : Key_Type; Right : Cursor) return Boolean is
    begin
        return Stored_Key (Right) < Left;
    end ">";

    function "=" (Left, Right : Map) return Boolean is
        P : Cursor := First (Left);
        Q : Cursor;
    begin
        if Length (Left) /= Length (Right) then return False; end if;
        while Has_Element (P) loop
            Q := Find (Right, Key (P));
            if not Has_Element (Q) or else not (Element (P) = Element (Q)) then return False; end if;
            Next (P);
        end loop;
        return True;
    end "=";

end Ada.Containers.Indefinite_Ordered_Maps;
