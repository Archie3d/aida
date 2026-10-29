-- Ada 2005 container implementation; ownership and cursor machinery is shared.
package body Ada.Containers.Indefinite_Ordered_Sets is
    use type Storage.Position;
    use type Storage.Element_Access;

    function Box_Key (Value : Natural) return Key_Box is
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

    function Length (Container : Set) return Count_Type is
    begin
        return Storage.Length (Container.Data);
    end Length;

    function Is_Empty (Container : Set) return Boolean is
    begin
        return Length (Container) = 0;
    end Is_Empty;

    procedure Clear (Container : in out Set) is
    begin
        Storage.Clear (Container.Data);
    end Clear;

    procedure Move (Target : in out Set; Source : in out Set) is
    begin
        Storage.Move (Target.Data, Source.Data);
    end Move;

    function First (Container : Set) return Cursor is
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

    function Equivalent_Elements (Left, Right : Element_Type) return Boolean is
    begin
        return not (Left < Right) and not (Right < Left);
    end Equivalent_Elements;

    function Find (Container : Set; Item : Element_Type) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
    begin
        Storage.Lock (Container.Data, True);
        begin
            while Has_Element (P) loop
                if Equivalent_Elements (Element (P), Item) then exit; end if;
                if Item < Element (P) then P := Wrap (P.Ptr.Left);
                else P := Wrap (P.Ptr.Right);
                end if;
            end loop;
        exception
            when others => Storage.Unlock (Container.Data, True); raise;
        end;
        Storage.Unlock (Container.Data, True);
        return P;
    end Find;

    function Contains (Container : Set; Item : Element_Type) return Boolean is
    begin
        return Has_Element (Find (Container, Item));
    end Contains;

    function Insertion_Point (Container : Set; Item : Element_Type; Skip : Cursor := No_Element) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
        Candidate : Cursor;
    begin
        while Has_Element (P) loop
            if Item < Element (P) then
                Candidate := P;
                P := Wrap (P.Ptr.Left);
            else
                P := Wrap (P.Ptr.Right);
            end if;
        end loop;
        if Candidate = Skip then return Next (Candidate); end if;
        return Candidate;
    end Insertion_Point;

    procedure Iterate (Container : Set; Process : not null access procedure (Position : Cursor)) is
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

    procedure Reverse_Iterate (Container : Set; Process : not null access procedure (Position : Cursor)) is
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

    procedure Query_Element (Position : Cursor; Process : not null access procedure (Element : Element_Type)) is
        Anchor : Storage.Position := Position.Ptr;
    begin
        Storage.Lock (Anchor, True);
        begin
            Process (Anchor.Item.all);
        exception
            when others => Storage.Unlock (Anchor, True); raise;
        end;
        Storage.Unlock (Anchor, True);
    end Query_Element;

    procedure Replace_Element (Container : in out Set; Position : Cursor; New_Item : Element_Type) is
        Anchor : Storage.Position := Position.Ptr;
        Other, Before : Cursor;
        Pending : Storage.Position := null;
        Old : Storage.Element_Access;
    begin
        Storage.Check_Cursors (Container.Data);
        Storage.Check_Elements (Container.Data);
        Storage.Check_Owner (Container.Data, Anchor);
        Other := Find (Container, New_Item);
        if Has_Element (Other) and then Other /= Position then raise Program_Error with "duplicate set element"; end if;
        Pending := Storage.Create (New_Item);

        Before := Insertion_Point (Container, New_Item, Position);
        Storage.Unlink (Container.Data, Anchor);
        Old := Anchor.Item;
        Anchor.Item := Pending.Item;
        Pending.Item := Old;
        Anchor.Hash := Pending.Hash;
        Storage.Link (Container.Data, Before.Ptr, Anchor);
        Storage.Destroy (Pending);
    exception
        when others => Storage.Destroy (Pending); raise;
    end Replace_Element;

    procedure Insert (Container : in out Set; New_Item : Element_Type; Position : out Cursor; Inserted : out Boolean) is
        Pending : Storage.Position := null;
        Before : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        Position := Find (Container, New_Item);
        Inserted := not Has_Element (Position);
        if not Inserted then return; end if;
        Before := Insertion_Point (Container, New_Item);
        Pending := Storage.Create (New_Item);


        Storage.Link (Container.Data, Before.Ptr, Pending);
        Position := Wrap (Pending);
        Pending := null;
    exception
        when others => Storage.Destroy (Pending); raise;
    end Insert;

    procedure Insert (Container : in out Set; New_Item : Element_Type) is
        P : Cursor;
        Inserted : Boolean;
    begin
        Insert (Container, New_Item, P, Inserted);
        if not Inserted then raise Constraint_Error with "duplicate container key"; end if;
    end Insert;

    procedure Include (Container : in out Set; New_Item : Element_Type) is
        P : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        P := Find (Container, New_Item);
        if not Has_Element (P) then
            Insert (Container, New_Item);
        else

            Replace_Element (Container, P, New_Item);
        end if;
    end Include;

    procedure Replace (Container : in out Set; New_Item : Element_Type) is
        P : Cursor;
    begin
        Storage.Check_Cursors (Container.Data);
        P := Find (Container, New_Item);
        if not Has_Element (P) then
            raise Constraint_Error with "key not found";
        else

            Replace_Element (Container, P, New_Item);
        end if;
    end Replace;

    procedure Delete (Container : in out Set; Position : in out Cursor) is
    begin
        Storage.Delete (Container.Data, Position.Ptr);
    end Delete;

    procedure Delete (Container : in out Set; Item : Element_Type) is
        P : Cursor := Find (Container, Item);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P);
        else raise Constraint_Error with "key not found";
        end if;
    end Delete;

    procedure Exclude (Container : in out Set; Item : Element_Type) is
        P : Cursor := Find (Container, Item);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P);

        end if;
    end Exclude;

    function Last (Container : Set) return Cursor is
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

    function First_Element (Container : Set) return Element_Type is
    begin
        return Element (First (Container));
    end First_Element;

    procedure Delete_First (Container : in out Set) is
        P : Cursor := First (Container);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P); end if;
    end Delete_First;

    function Last_Element (Container : Set) return Element_Type is
    begin
        return Element (Last (Container));
    end Last_Element;

    procedure Delete_Last (Container : in out Set) is
        P : Cursor := Last (Container);
    begin
        Storage.Check_Cursors (Container.Data);
        if Has_Element (P) then Delete (Container, P); end if;
    end Delete_Last;

    function Floor (Container : Set; Item : Element_Type) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
        Candidate : Cursor;
    begin
        Storage.Lock (Container.Data, True);
        begin
            while Has_Element (P) loop
                if Item < Element (P) then
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

    function Ceiling (Container : Set; Item : Element_Type) return Cursor is
        P : Cursor := Wrap (Storage.Root (Container.Data));
        Candidate : Cursor;
    begin
        Storage.Lock (Container.Data, True);
        begin
            while Has_Element (P) loop
                if Element (P) < Item then
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
        return Element (Left) < Element (Right);
    end "<";

    function "<" (Left : Cursor; Right : Element_Type) return Boolean is
    begin
        return Element (Left) < Right;
    end "<";

    function "<" (Left : Element_Type; Right : Cursor) return Boolean is
    begin
        return Left < Element (Right);
    end "<";

    function ">" (Left : Cursor; Right : Cursor) return Boolean is
    begin
        return Element (Right) < Element (Left);
    end ">";

    function ">" (Left : Cursor; Right : Element_Type) return Boolean is
    begin
        return Right < Element (Left);
    end ">";

    function ">" (Left : Element_Type; Right : Cursor) return Boolean is
    begin
        return Element (Right) < Left;
    end ">";

    function To_Set (New_Item : Element_Type) return Set is
        Result : Set;
    begin
        Insert (Result, New_Item);
        return Result;
    end To_Set;

    function "=" (Left, Right : Set) return Boolean is
        P : Cursor := First (Left);
        Q : Cursor;
    begin
        if Length (Left) /= Length (Right) then return False; end if;
        while Has_Element (P) loop
            Q := Find (Right, Element (P));
            if not Has_Element (Q) then return False; end if;
            if not (Element (P) = Element (Q)) then return False; end if;
            Next (P);
        end loop;
        return True;
    end "=";

    function Equivalent_Sets (Left, Right : Set) return Boolean is
        P : Cursor := First (Left);
        Q : Cursor;
    begin
        if Length (Left) /= Length (Right) then return False; end if;
        while Has_Element (P) loop
            Q := Find (Right, Element (P));
            if not Has_Element (Q) then return False; end if;

            Next (P);
        end loop;
        return True;
    end Equivalent_Sets;

    function Overlap (Left, Right : Set) return Boolean is
        P : Cursor := First (Left);
    begin
        while Has_Element (P) loop
            if Contains (Right, Element (P)) then return True; end if;
            Next (P);
        end loop;
        return False;
    end Overlap;

    function Is_Subset (Subset : Set; Of_Set : Set) return Boolean is
        P : Cursor := First (Subset);
    begin
        while Has_Element (P) loop
            if not Contains (Of_Set, Element (P)) then return False; end if;
            Next (P);
        end loop;
        return True;
    end Is_Subset;

    procedure Union (Target : in out Set; Source : Set) is
        P : Cursor := First (Source);
    begin
        Storage.Check_Cursors (Target.Data);
        while Has_Element (P) loop
            if not Contains (Target, Element (P)) then Insert (Target, Element (P)); end if;
            Next (P);
        end loop;
    end Union;

    function Union (Left, Right : Set) return Set is
        Result : Set := Left;
    begin
        Union (Result, Right);
        return Result;
    end Union;

    procedure Intersection (Target : in out Set; Source : Set) is
        P : Cursor := First (Target);
        N : Cursor;
    begin
        Storage.Check_Cursors (Target.Data);
        if Target'Address = Source'Address then
            return;
        end if;
        while Has_Element (P) loop
            N := Next (P);
            if not Contains (Source, Element (P)) then Delete (Target, P); end if;
            P := N;
        end loop;
    end Intersection;

    function Intersection (Left, Right : Set) return Set is
        Result : Set := Left;
    begin
        Intersection (Result, Right);
        return Result;
    end Intersection;

    procedure Difference (Target : in out Set; Source : Set) is
        P : Cursor := First (Target);
        N : Cursor;
    begin
        Storage.Check_Cursors (Target.Data);
        if Target'Address = Source'Address then
            Clear (Target); return;
        end if;
        while Has_Element (P) loop
            N := Next (P);
            if Contains (Source, Element (P)) then Delete (Target, P); end if;
            P := N;
        end loop;
    end Difference;

    function Difference (Left, Right : Set) return Set is
        Result : Set := Left;
    begin
        Difference (Result, Right);
        return Result;
    end Difference;

    procedure Symmetric_Difference (Target : in out Set; Source : Set) is
        P : Cursor := First (Source);
        Q : Cursor;
    begin
        Storage.Check_Cursors (Target.Data);
        if Target'Address = Source'Address then Clear (Target); return; end if;
        while Has_Element (P) loop
            Q := Find (Target, Element (P));
            if Has_Element (Q) then Delete (Target, Q); else Insert (Target, Element (P)); end if;
            Next (P);
        end loop;
    end Symmetric_Difference;

    function Symmetric_Difference (Left, Right : Set) return Set is
        Result : Set := Left;
    begin
        Symmetric_Difference (Result, Right);
        return Result;
    end Symmetric_Difference;

    package body Generic_Keys is
        function Equivalent_Keys (Left, Right : Key_Type) return Boolean is
        begin
            return not (Left < Right) and not (Right < Left);
        end Equivalent_Keys;

        function Key (Position : Cursor) return Key_Type is
        begin
            if Position.Ptr = null then raise Constraint_Error; end if;
            return Key (Position.Ptr.Item.all);
        end Key;

        function Value (Position : Cursor) return Element_Type is
        begin
            if Position.Ptr = null then raise Constraint_Error; end if;
            return Position.Ptr.Item.all;
        end Value;

        function Projected_Key (Position : Cursor) return Key_Type is
        begin
            return Key (Position);
        end Projected_Key;

        function Find (Container : Set; Key : Key_Type) return Cursor is
            P : Cursor := Wrap (Storage.Root (Container.Data));
        begin
            while Has_Element (P) loop
                if Equivalent_Keys (Key, Projected_Key (P)) then return P; end if;
                if Key < Projected_Key (P) then P := Wrap (P.Ptr.Left);
                else P := Wrap (P.Ptr.Right);
                end if;
            end loop;
            return No_Element;
        end Find;

        function Element (Container : Set; Key : Key_Type) return Element_Type is
        begin
            return Value (Find (Container, Key));
        end Element;

        function Contains (Container : Set; Key : Key_Type) return Boolean is
        begin
            return Has_Element (Find (Container, Key));
        end Contains;

        procedure Replace (Container : in out Set; Key : Key_Type; New_Item : Element_Type) is
        begin
            Replace_Element (Container, Find (Container, Key), New_Item);
        end Replace;

        procedure Delete (Container : in out Set; Key : Key_Type) is
            P : Cursor := Find (Container, Key);
        begin
            Storage.Check_Cursors (Container.Data);
            if Has_Element (P) then Storage.Delete (Container.Data, P.Ptr);
            else raise Constraint_Error;
            end if;
        end Delete;

        procedure Exclude (Container : in out Set; Key : Key_Type) is
            P : Cursor := Find (Container, Key);
        begin
            Storage.Check_Cursors (Container.Data);
            if Has_Element (P) then Storage.Delete (Container.Data, P.Ptr);

            end if;
        end Exclude;

        function Floor (Container : Set; Key : Key_Type) return Cursor is
            P : Cursor := Wrap (Storage.Root (Container.Data));
            Candidate : Cursor;
        begin
            Storage.Lock (Container.Data, True);
            begin
                while Has_Element (P) loop
                    if Key < Projected_Key (P) then
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

        function Ceiling (Container : Set; Key : Key_Type) return Cursor is
            P : Cursor := Wrap (Storage.Root (Container.Data));
            Candidate : Cursor;
        begin
            Storage.Lock (Container.Data, True);
            begin
                while Has_Element (P) loop
                    if Projected_Key (P) < Key then
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

        procedure Update_Element_Preserving_Key (Container : in out Set; Position : Cursor;
            Process : not null access procedure (Element : in out Element_Type)) is
            Anchor : Storage.Position := Position.Ptr;
            P : Cursor := Position;
            Changed : Boolean := False;
        begin
            Storage.Check_Owner (Container.Data, Anchor);
            Storage.Check_Elements (Container.Data);
            Storage.Lock (Anchor, True);
            begin
                declare
                    Saved : Key_Type := Key (P);
                begin
                    Process (Anchor.Item.all);
                    Changed := not Equivalent_Keys (Saved, Key (P));
                end;
            exception
                when others => Storage.Unlock (Anchor, True); raise;
            end;
            Storage.Unlock (Anchor, True);
            if Changed then
                Storage.Delete (Container.Data, P.Ptr);
                raise Program_Error with "set key changed during update";
            end if;
        end Update_Element_Preserving_Key;

    end Generic_Keys;
end Ada.Containers.Indefinite_Ordered_Sets;
