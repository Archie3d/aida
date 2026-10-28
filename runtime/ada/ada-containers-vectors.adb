with Ada.Unchecked_Deallocation;
package body Ada.Containers.Vectors is
    procedure Free_Element is new Ada.Unchecked_Deallocation (Element_Type, Element_Access);
    procedure Free_Slots is new Ada.Unchecked_Deallocation (Slots, Slots_Access);

    function Maximum_Length return Count_Type is
        First : Long_Integer := Long_Integer (Index_Type'First);
        Last : Long_Integer := Long_Integer (Index_Type'Last);
        Limit : constant Long_Integer := Long_Integer (Count_Type'Last);
    begin
        if Last < First then
            return 0;
        end if;
        -- Avoid overflowing a signed 64-bit difference for wide index types.
        if Last >= Long_Integer'First + Limit then
            if First <= Last - Limit then
                return Count_Type'Last;
            end if;
        end if;
        return Count_Type (Last - First + 1);
    end Maximum_Length;

    procedure Check_Length (Size : Count_Type) is
    begin
        if Size > Maximum_Length then
            raise Constraint_Error with "vector index range exhausted";
        end if;
    end Check_Length;

    function Position (Index : Extended_Index) return Count_Type is
        Offset : Long_Integer := Long_Integer (Index) - Long_Integer (Index_Type'First);
    begin
        if Offset < 0 or else Offset >= Long_Integer (Count_Type'Last) then
            raise Constraint_Error with "vector index out of range";
        end if;
        return Count_Type (Offset + 1);
    end Position;

    procedure Clear (Container : in out Vector) is
        Item : Element_Access;
        Failed : Boolean := False;
    begin
        while Container.Used > 0 loop
            Item := Container.Data.all (Container.Used);
            Container.Data.all (Container.Used) := null;
            Container.Used := Container.Used - 1;
            -- Detach before user finalization, and finish all elements even
            -- if several finalizers fail. Deallocation reports Program_Error.
            begin
                Free_Element (Item);
            exception
                when others => Failed := True;
            end;
        end loop;
        if Failed then
            raise Program_Error with "vector element finalization failed";
        end if;
    end Clear;

    procedure Finalize (Object : in out Vector) is
    begin
        Clear (Object);
        Free_Slots (Object.Data);
        Object.Allocated := 0;
    exception
        when others =>
            Free_Slots (Object.Data);
            Object.Allocated := 0;
            raise;
    end Finalize;

    procedure Adjust (Object : in out Vector) is
        Original : Slots_Access := Object.Data;
        Size : Count_Type := Object.Used;
    begin
        Object.Data := null;
        Object.Used := 0;
        Object.Allocated := 0;
        Reserve_Capacity (Object, Size);
        for I in 1 .. Size loop
            Object.Data.all (I) := new Element_Type'(Original.all (I).all);
            Object.Used := I;
        end loop;
    exception
        when others =>
            Finalize (Object);
            raise;
    end Adjust;

    function Capacity (Container : Vector) return Count_Type is
    begin
        return Container.Allocated;
    end Capacity;

    procedure Reserve_Capacity (Container : in out Vector; Capacity : Count_Type) is
        Replacement : Slots_Access;
    begin
        Check_Length (Capacity);
        if Capacity <= Container.Allocated then
            return;
        end if;
        Replacement := new Slots'(1 .. Capacity => null);
        for I in 1 .. Container.Used loop
            Replacement.all (I) := Container.Data.all (I);
        end loop;
        Free_Slots (Container.Data);
        Container.Data := Replacement;
        Container.Allocated := Capacity;
    end Reserve_Capacity;

    procedure Grow (Container : in out Vector; Needed : Count_Type) is
        Size : Count_Type := Container.Allocated;
    begin
        Check_Length (Needed);
        if Needed <= Size then
            return;
        end if;
        if Size <= Maximum_Length / 2 then
            Size := Size * 2;
        else
            Size := Maximum_Length;
        end if;
        if Size < Needed then
            Size := Needed;
        end if;
        Reserve_Capacity (Container, Size);
    end Grow;

    function Length (Container : Vector) return Count_Type is
    begin
        return Container.Used;
    end Length;

    function Is_Empty (Container : Vector) return Boolean is
    begin
        return Container.Used = 0;
    end Is_Empty;

    function Element (Container : Vector; Index : Index_Type) return Element_Type is
        Slot : Count_Type := Position (Index);
    begin
        if Slot > Container.Used then
            raise Constraint_Error;
        end if;
        return Container.Data.all (Slot).all;
    end Element;

    procedure Replace_Element (Container : in out Vector; Index : Index_Type; New_Item : Element_Type) is
        Slot : Count_Type := Position (Index);
    begin
        if Slot > Container.Used then
            raise Constraint_Error;
        end if;
        Container.Data.all (Slot).all := New_Item;
    end Replace_Element;

    function First_Index (Container : Vector) return Index_Type is
    begin
        return Index_Type'First;
    end First_Index;

    function Last_Index (Container : Vector) return Extended_Index is
    begin
        if Container.Used = 0 then
            return No_Index;
        end if;
        return Extended_Index (Long_Integer (Index_Type'First) + Long_Integer (Container.Used - 1));
    end Last_Index;

    function First_Element (Container : Vector) return Element_Type is
    begin
        return Element (Container, Index_Type'First);
    end First_Element;

    function Last_Element (Container : Vector) return Element_Type is
    begin
        if Container.Used = 0 then
            raise Constraint_Error;
        end if;
        return Container.Data.all (Container.Used).all;
    end Last_Element;

    function "=" (Left, Right : Vector) return Boolean is
    begin
        if Left.Used /= Right.Used then
            return False;
        end if;
        for I in 1 .. Left.Used loop
            if not (Left.Data.all (I).all = Right.Data.all (I).all) then
                return False;
            end if;
        end loop;
        return True;
    end "=";

    function To_Vector (Length : Count_Type) return Vector is
        Result : Vector;
    begin
        Reserve_Capacity (Result, Length);
        for I in 1 .. Length loop
            Result.Data.all (I) := new Element_Type;
            Result.Used := I;
        end loop;
        return Result;
    end To_Vector;

    function To_Vector (New_Item : Element_Type; Length : Count_Type) return Vector is
        Result : Vector;
    begin
        Append (Result, New_Item, Length);
        return Result;
    end To_Vector;

    procedure Insert (Container : in out Vector; Before : Extended_Index;
                      New_Item : Element_Type; Count : Count_Type := 1) is
        Slot : Count_Type := Position (Before);
        Pending : Vector;
    begin
        if Slot - 1 > Container.Used then
            raise Constraint_Error;
        end if;
        if Count > Maximum_Length - Container.Used then
            raise Constraint_Error;
        end if;
        -- Construct copies before moving slots: a failed Adjust leaves all
        -- existing elements intact, and Pending owns partial construction.
        Reserve_Capacity (Pending, Count);
        for I in 1 .. Count loop
            Pending.Data.all (I) := new Element_Type'(New_Item);
            Pending.Used := I;
        end loop;
        Grow (Container, Container.Used + Count);
        for I in reverse Slot .. Container.Used loop
            Container.Data.all (I + Count) := Container.Data.all (I);
        end loop;
        for I in 1 .. Count loop
            Container.Data.all (Slot + (I - 1)) := Pending.Data.all (I);
            Pending.Data.all (I) := null;
        end loop;
        Pending.Used := 0;
        Container.Used := Container.Used + Count;
    end Insert;

    procedure Append (Container : in out Vector; New_Item : Element_Type; Count : Count_Type := 1) is
        Before : Extended_Index;
    begin
        if Count = 0 then
            return;
        end if;
        if Count > Maximum_Length - Container.Used then
            raise Constraint_Error;
        end if;
        Before := Extended_Index (Long_Integer (Index_Type'First) + Long_Integer (Container.Used));
        Insert (Container, Before, New_Item, Count);
    end Append;

    procedure Append (Container : in out Vector; New_Item : Vector) is
        Pending : Vector := New_Item;
        Old_Length : Count_Type := Container.Used;
    begin
        if Pending.Used > Maximum_Length - Container.Used then
            raise Constraint_Error;
        end if;
        Grow (Container, Old_Length + Pending.Used);
        for I in 1 .. Pending.Used loop
            Container.Data.all (Old_Length + I) := Pending.Data.all (I);
            Pending.Data.all (I) := null;
        end loop;
        Container.Used := Old_Length + Pending.Used;
        Pending.Used := 0;
    end Append;

    procedure Prepend (Container : in out Vector; New_Item : Element_Type; Count : Count_Type := 1) is
    begin
        Insert (Container, Index_Type'First, New_Item, Count);
    end Prepend;

    procedure Delete (Container : in out Vector; Index : Extended_Index; Count : Count_Type := 1) is
        Slot : Count_Type := Position (Index);
        Amount : Count_Type;
        Removed : Vector;
    begin
        if Slot - 1 > Container.Used then
            raise Constraint_Error;
        end if;
        Amount := Container.Used - (Slot - 1);
        if Amount > Count then
            Amount := Count;
        end if;
        if Amount = 0 then
            return;
        end if;
        Reserve_Capacity (Removed, Amount);
        for I in 1 .. Amount loop
            Removed.Data.all (I) := Container.Data.all (Slot + (I - 1));
        end loop;
        Removed.Used := Amount;
        for I in Slot .. Container.Used - Amount loop
            Container.Data.all (I) := Container.Data.all (I + Amount);
        end loop;
        for I in Container.Used - Amount + 1 .. Container.Used loop
            Container.Data.all (I) := null;
        end loop;
        Container.Used := Container.Used - Amount;
    end Delete;

    procedure Delete_First (Container : in out Vector; Count : Count_Type := 1) is
    begin
        Delete (Container, Index_Type'First, Count);
    end Delete_First;

    procedure Delete_Last (Container : in out Vector; Count : Count_Type := 1) is
        Amount : Count_Type := Count;
    begin
        if Amount > Container.Used then
            Amount := Container.Used;
        end if;
        if Amount > 0 then
            Delete (Container, Extended_Index (Long_Integer (Index_Type'First) +
                Long_Integer (Container.Used - Amount)), Amount);
        end if;
    end Delete_Last;

    procedure Set_Length (Container : in out Vector; Length : Count_Type) is
        Pending : Vector;
        Old_Length : Count_Type := Container.Used;
    begin
        if Length <= Old_Length then
            Delete_Last (Container, Old_Length - Length);
            return;
        end if;
        Check_Length (Length);
        Pending := To_Vector (Length - Old_Length);
        Grow (Container, Length);
        for I in 1 .. Pending.Used loop
            Container.Data.all (Old_Length + I) := Pending.Data.all (I);
            Pending.Data.all (I) := null;
        end loop;
        Pending.Used := 0;
        Container.Used := Length;
    end Set_Length;

    procedure Swap (Container : in out Vector; I, J : Index_Type) is
        Left : Count_Type := Position (I);
        Right : Count_Type := Position (J);
        Temp : Element_Access;
    begin
        if Left > Container.Used or Right > Container.Used then
            raise Constraint_Error;
        end if;
        Temp := Container.Data.all (Left);
        Container.Data.all (Left) := Container.Data.all (Right);
        Container.Data.all (Right) := Temp;
    end Swap;

    procedure Reverse_Elements (Container : in out Vector) is
        Temp : Element_Access;
        J : Count_Type;
    begin
        for I in 1 .. Container.Used / 2 loop
            J := Container.Used - I + 1;
            Temp := Container.Data.all (I);
            Container.Data.all (I) := Container.Data.all (J);
            Container.Data.all (J) := Temp;
        end loop;
    end Reverse_Elements;

    procedure Move (Target : in out Vector; Source : in out Vector) is
    begin
        if Target'Address = Source'Address then
            return;
        end if;
        Finalize (Target);
        Target.Data := Source.Data;
        Target.Used := Source.Used;
        Target.Allocated := Source.Allocated;
        Source.Data := null;
        Source.Used := 0;
        Source.Allocated := 0;
    end Move;
end Ada.Containers.Vectors;
