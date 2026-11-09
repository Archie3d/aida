with Ada.Unchecked_Deallocation;
package body Ada.Containers.Storage is
    procedure Free_Key is new Ada.Unchecked_Deallocation (Key_Box, Key_Access);
    procedure Free_Item is new Ada.Unchecked_Deallocation (Element_Box, Element_Access);
    procedure Free_Node is new Ada.Unchecked_Deallocation (Node, Position);
    procedure Free_Table is new Ada.Unchecked_Deallocation (Table, Table_Access);
    procedure Free_State is new Ada.Unchecked_Deallocation (State, State_Access);

    procedure Ensure (Object : in out Store) is
    begin
        if Object.Data = null then
            Object.Data := new State;
        end if;
    end Ensure;

    function Length (Object : Store) return Count_Type is
    begin
        if Object.Data = null then return 0; end if;
        return Object.Data.Used;
    end Length;

    function Capacity (Object : Store) return Count_Type is
    begin
        if Object.Data = null then return 0; end if;
        return Object.Data.Allocated;
    end Capacity;

    function Root (Object : Store) return Position is
    begin
        if Object.Data = null then return null; end if;
        return Object.Data.Root;
    end Root;

    function First (Object : Store) return Position is
    begin
        if Object.Data = null then return null; end if;
        return Object.Data.First;
    end First;

    function Last (Object : Store) return Position is
    begin
        if Object.Data = null then return null; end if;
        return Object.Data.Last;
    end Last;

    function At_Index (Object : Store; Index : Count_Type) return Position is
    begin
        if Index = 0 or else Index > Length (Object) then
            raise Constraint_Error with "container index out of range";
        end if;
        return Object.Data.Slots.all (Index);
    end At_Index;

    procedure Check_Owner (Object : Store; Item : Position; Allow_Null : Boolean := False) is
    begin
        if Item = null then
            if not Allow_Null then raise Constraint_Error with "No_Element"; end if;
        elsif Item.Owner /= Object.Data then
            raise Program_Error with "cursor belongs to another container";
        end if;
    end Check_Owner;

    procedure Check_Cursors (Object : Store) is
    begin
        if Object.Data /= null and then Object.Data.Cursors_Busy > 0 then
            raise Program_Error with "tampering with cursors";
        end if;
    end Check_Cursors;

    procedure Check_Elements (Object : Store) is
    begin
        if Object.Data /= null and then Object.Data.Elements_Busy > 0 then
            raise Program_Error with "tampering with elements";
        end if;
    end Check_Elements;

    procedure Lock (Object : Store; Elements : Boolean := False) is
    begin
        if Object.Data /= null then
            Object.Data.Cursors_Busy := Object.Data.Cursors_Busy + 1;
            if Elements then Object.Data.Elements_Busy := Object.Data.Elements_Busy + 1; end if;
        end if;
    end Lock;

    procedure Unlock (Object : Store; Elements : Boolean := False) is
    begin
        if Object.Data /= null then
            Object.Data.Cursors_Busy := Object.Data.Cursors_Busy - 1;
            if Elements then Object.Data.Elements_Busy := Object.Data.Elements_Busy - 1; end if;
        end if;
    end Unlock;

    procedure Lock (Item : Position; Elements : Boolean := False) is
    begin
        if Item = null then raise Constraint_Error with "No_Element"; end if;
        Item.Owner.Cursors_Busy := Item.Owner.Cursors_Busy + 1;
        if Elements then Item.Owner.Elements_Busy := Item.Owner.Elements_Busy + 1; end if;
    end Lock;

    procedure Unlock (Item : Position; Elements : Boolean := False) is
    begin
        Item.Owner.Cursors_Busy := Item.Owner.Cursors_Busy - 1;
        if Elements then Item.Owner.Elements_Busy := Item.Owner.Elements_Busy - 1; end if;
    end Unlock;

    function Bucket (Hash : Hash_Type; Size : Count_Type) return Count_Type is
    begin
        return Count_Type (Hash mod Hash_Type (Size)) + 1;
    end Bucket;

    procedure Reserve (Object : in out Store; Size : Count_Type) is
        Slots : Table_Access;
        Item : Position;
        I : Count_Type;
    begin
        if Size <= Capacity (Object) then return; end if;
        Ensure (Object);
        Slots := new Table'(1 .. Size => null);
        Item := First (Object);
        while Item /= null loop
            if Indexed then
                Slots.all (Item.Index) := Item;
            elsif Hashed then
                I := Bucket (Item.Hash, Size);
                Item.Chain := Slots.all (I);
                Slots.all (I) := Item;
            end if;
            Item := Item.Next;
        end loop;
        Free_Table (Object.Data.Slots);
        Object.Data.Slots := Slots;
        Object.Data.Allocated := Size;
    end Reserve;

    function Create (Item : Element_Type) return Position is
        Result : Position := new Node;
    begin
        Result.Item := new Element_Box'(Box_Element (Item));
        return Result;
    exception
        when others => Destroy (Result); raise;
    end Create;

    procedure Set_Key (Item : Position; Key : Key_Type) is
        Replacement : Key_Access := new Key_Box'(Box_Key (Key));
        Old : Key_Access := Item.Key;
    begin
        Item.Key := Replacement;
        Free_Key (Old);
    end Set_Key;

    procedure Replace (Item : Position; Value : Element_Type) is
        Replacement : Element_Access;
        Old : Element_Access;
    begin
        if Item = null then raise Constraint_Error with "No_Element"; end if;
        if Item.Owner.Elements_Busy > 0 then raise Program_Error with "tampering with elements"; end if;
        Replacement := new Element_Box'(Box_Element (Value));
        Old := Item.Item;
        Item.Item := Replacement;
        Free_Item (Old);
    end Replace;

    procedure Destroy (Item : in out Position) is
        Old : Position := Item;
        Failed : Boolean := False;
    begin
        Item := null;
        if Old = null then return; end if;
        begin Free_Key (Old.Key); exception when others => Failed := True; end;
        begin Free_Item (Old.Item); exception when others => Failed := True; end;
        Free_Node (Old);
        if Failed then raise Program_Error with "container element finalization failed"; end if;
    end Destroy;

    function Height (Item : Position) return Integer is
    begin
        if Item = null then return 0; end if;
        return Item.Height;
    end Height;

    procedure Update_Height (Item : Position) is
        H : Integer := Height (Item.Left);
    begin
        if Height (Item.Right) > H then H := Height (Item.Right); end if;
        Item.Height := H + 1;
    end Update_Height;

    procedure Set_Parent (Object : in out Store; Old, Replacement : Position) is
    begin
        if Replacement /= null then Replacement.Parent := Old.Parent; end if;
        if Old.Parent = null then Object.Data.Root := Replacement;
        elsif Old.Parent.Left = Old then Old.Parent.Left := Replacement;
        else Old.Parent.Right := Replacement;
        end if;
    end Set_Parent;

    function Rotate_Left (Object : in out Store; Item : Position) return Position is
        Top : Position := Item.Right;
    begin
        Set_Parent (Object, Item, Top);
        Item.Right := Top.Left;
        if Item.Right /= null then Item.Right.Parent := Item; end if;
        Top.Left := Item;
        Item.Parent := Top;
        Update_Height (Item);
        Update_Height (Top);
        return Top;
    end Rotate_Left;

    function Rotate_Right (Object : in out Store; Item : Position) return Position is
        Top : Position := Item.Left;
    begin
        Set_Parent (Object, Item, Top);
        Item.Left := Top.Right;
        if Item.Left /= null then Item.Left.Parent := Item; end if;
        Top.Right := Item;
        Item.Parent := Top;
        Update_Height (Item);
        Update_Height (Top);
        return Top;
    end Rotate_Right;

    procedure Rebalance (Object : in out Store; Start : Position) is
        Item : Position := Start;
        Temp : Position;
    begin
        while Item /= null loop
            Update_Height (Item);
            if Height (Item.Left) - Height (Item.Right) > 1 then
                if Height (Item.Left.Left) < Height (Item.Left.Right) then
                    Temp := Rotate_Left (Object, Item.Left);
                end if;
                Item := Rotate_Right (Object, Item);
            elsif Height (Item.Right) - Height (Item.Left) > 1 then
                if Height (Item.Right.Right) < Height (Item.Right.Left) then
                    Temp := Rotate_Right (Object, Item.Right);
                end if;
                Item := Rotate_Left (Object, Item);
            end if;
            Item := Item.Parent;
        end loop;
    end Rebalance;

    procedure Link (Object : in out Store; Before : Position; Item : Position) is
        Size : Count_Type;
        I : Count_Type;
    begin
        Ensure (Object);
        if Object.Data.Used = Count_Type'Last then raise Constraint_Error; end if;
        if (Indexed or Hashed) and then Object.Data.Used = Capacity (Object) then
            Size := Capacity (Object);
            if Size < 4 then Size := 4;
            elsif Size <= Count_Type'Last / 2 then Size := Size * 2;
            else Size := Count_Type'Last;
            end if;
            Reserve (Object, Size);
        end if;
        if Ordered then
            Item.Left := null;
            Item.Right := null;
            Item.Parent := null;
            Item.Height := 1;
            if Before = null then
                Item.Parent := Object.Data.Last;
                if Item.Parent /= null then Item.Parent.Right := Item; end if;
            elsif Before.Left = null then
                Before.Left := Item;
                Item.Parent := Before;
            else
                Item.Parent := Before.Prev;
                Item.Parent.Right := Item;
            end if;
            if Item.Parent = null then Object.Data.Root := Item; end if;
            Rebalance (Object, Item);
        end if;
        Item.Owner := Object.Data;
        Item.Next := Before;
        if Before = null then Item.Prev := Object.Data.Last;
        else Item.Prev := Before.Prev;
        end if;
        if Item.Prev = null then Object.Data.First := Item;
        else Item.Prev.Next := Item;
        end if;
        if Before = null then Object.Data.Last := Item;
        else Before.Prev := Item;
        end if;
        Object.Data.Used := Object.Data.Used + 1;
        if Indexed then
            if Before = null then I := Object.Data.Used; else I := Before.Index; end if;
            for J in reverse I .. Object.Data.Used - 1 loop
                Object.Data.Slots.all (J + 1) := Object.Data.Slots.all (J);
                Object.Data.Slots.all (J + 1).Index := J + 1;
            end loop;
            Object.Data.Slots.all (I) := Item;
            Item.Index := I;
        elsif Hashed then
            I := Bucket (Item.Hash, Object.Data.Allocated);
            Item.Chain := Object.Data.Slots.all (I);
            Object.Data.Slots.all (I) := Item;
        end if;
    end Link;

    procedure Unlink (Object : in out Store; Item : Position) is
        I : Count_Type;
        P : Position;
        Temp : Position;
    begin
        if Ordered then
            -- Transplant the successor node, retaining every other node's
            -- address and value. Rebalance from the successor's old parent.
            if Item.Left = null then
                P := Item.Parent;
                Set_Parent (Object, Item, Item.Right);
            elsif Item.Right = null then
                P := Item.Parent;
                Set_Parent (Object, Item, Item.Left);
            else
                Temp := Item.Next;
                if Temp.Parent = Item then
                    P := Temp;
                else
                    P := Temp.Parent;
                    Set_Parent (Object, Temp, Temp.Right);
                    Temp.Right := Item.Right;
                    Temp.Right.Parent := Temp;
                end if;
                Set_Parent (Object, Item, Temp);
                Temp.Left := Item.Left;
                Temp.Left.Parent := Temp;
                Temp.Height := Item.Height;
            end if;
            Rebalance (Object, P);
            Item.Left := null;
            Item.Right := null;
            Item.Parent := null;
        end if;
        if Indexed then
            for J in Item.Index .. Object.Data.Used - 1 loop
                Object.Data.Slots.all (J) := Object.Data.Slots.all (J + 1);
                Object.Data.Slots.all (J).Index := J;
            end loop;
            Object.Data.Slots.all (Object.Data.Used) := null;
        elsif Hashed then
            I := Bucket (Item.Hash, Object.Data.Allocated);
            P := Object.Data.Slots.all (I);
            if P = Item then Object.Data.Slots.all (I) := Item.Chain;
            else
                while P.Chain /= Item loop P := P.Chain; end loop;
                P.Chain := Item.Chain;
            end if;
        end if;
        if Item.Prev = null then Object.Data.First := Item.Next;
        else Item.Prev.Next := Item.Next;
        end if;
        if Item.Next = null then Object.Data.Last := Item.Prev;
        else Item.Next.Prev := Item.Prev;
        end if;
        Object.Data.Used := Object.Data.Used - 1;
        Item.Owner := null;
        Item.Next := null;
        Item.Prev := null;
        Item.Chain := null;
    end Unlink;

    procedure Delete (Object : in out Store; Item : in out Position) is
    begin
        Check_Cursors (Object);
        Check_Owner (Object, Item);
        Unlink (Object, Item);
        Destroy (Item);
    end Delete;

    procedure Clear (Object : in out Store) is
        Item : Position;
        Failed : Boolean := False;
    begin
        Check_Cursors (Object);
        while First (Object) /= null loop
            Item := Last (Object);
            Unlink (Object, Item);
            begin Destroy (Item); exception when others => Failed := True; end;
        end loop;
        if Failed then raise Program_Error with "container finalization failed"; end if;
    end Clear;

    procedure Finalize (Object : in out Store) is
    begin
        Check_Cursors (Object);
        begin
            Clear (Object);
            if Object.Data /= null then
                Free_Table (Object.Data.Slots);
                Free_State (Object.Data);
            end if;
        exception
            when others =>
                if Object.Data /= null then
                    Free_Table (Object.Data.Slots);
                    Free_State (Object.Data);
                end if;
                raise;
        end;
    end Finalize;

    procedure Adjust (Object : in out Store) is
        Original : State_Access := Object.Data;
        Source : Position;
        Item : Position := null;
    begin
        Object.Data := null;
        if Original = null then return; end if;
        Source := Original.First;
        while Source /= null loop
            Item := new Node;
            if Source.Key /= null then Item.Key := new Key_Box'(Source.Key.all); end if;
            if Source.Item /= null then Item.Item := new Element_Box'(Source.Item.all); end if;
            Item.Hash := Source.Hash;
            Link (Object, null, Item);
            Item := null;
            Source := Source.Next;
        end loop;
    exception
        when others =>
            begin Destroy (Item); exception when others => null; end;
            Finalize (Object);
            raise;
    end Adjust;

    procedure Move (Target : in out Store; Source : in out Store) is
    begin
        Check_Cursors (Target);
        Check_Cursors (Source);
        if Target'Address = Source'Address then return; end if;
        Clear (Target);
        if Target.Data /= null then
            Free_Table (Target.Data.Slots);
            Free_State (Target.Data);
        end if;
        Target.Data := Source.Data;
        Source.Data := null;
    end Move;

    procedure Swap_Items (Left, Right : Position) is
        Item : Element_Access;
    begin
        if Left = null or Right = null then raise Constraint_Error; end if;
        if Left.Item = null or Right.Item = null then raise Constraint_Error with "empty vector element"; end if;
        if Left.Owner.Elements_Busy > 0 or Right.Owner.Elements_Busy > 0 then
            raise Program_Error with "tampering with elements";
        end if;
        Item := Left.Item;
        Left.Item := Right.Item;
        Right.Item := Item;
    end Swap_Items;

    procedure Swap_Links (Object : in out Store; Left, Right : Position) is
        Before : Position;
    begin
        if Left = Right then return; end if;
        if Right.Next = Left then Swap_Links (Object, Right, Left); return; end if;
        Before := Right.Next;
        Unlink (Object, Right);
        Link (Object, Left, Right);
        Unlink (Object, Left);
        Link (Object, Before, Left);
    end Swap_Links;

    procedure Reverse_Links (Object : in out Store) is
        Item : Position := First (Object);
        Temp : Position;
    begin
        while Item /= null loop
            Temp := Item.Next;
            Item.Next := Item.Prev;
            Item.Prev := Temp;
            Item := Temp;
        end loop;
        Temp := Object.Data.First;
        Object.Data.First := Object.Data.Last;
        Object.Data.Last := Temp;
    end Reverse_Links;

    function Hash_First (Object : Store; Hash : Hash_Type) return Position is
    begin
        if Capacity (Object) = 0 then return null; end if;
        return Object.Data.Slots.all (Bucket (Hash, Capacity (Object)));
    end Hash_First;
end Ada.Containers.Storage;
