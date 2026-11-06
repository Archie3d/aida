with Ada.Text_IO;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with System.Address_To_Access_Conversions;
with System.Storage_Elements;
with Address_Views;
procedure AddressConversions is
    use type System.Address;
    generic package Views renames System.Address_To_Access_Conversions;
    package Integers is new Views (Integer);
    package Other_Integers is new Views (Integer);
    use type Integers.Object_Pointer;
    use type Other_Integers.Object_Pointer;
    use type Address_Views.Integers.Object_Pointer;
    function Pointer (Value : System.Address) return Integers.Object_Pointer renames Integers.To_Pointer;
    function Address (Value : Integers.Object_Pointer) return System.Address renames Integers.To_Address;
    generic
        with function Cast (Value : System.Address) return Integers.Object_Pointer is Integers.To_Pointer;
    function Apply (Value : System.Address) return Integers.Object_Pointer;
    function Apply (Value : System.Address) return Integers.Object_Pointer is
    begin
        return Cast (Value);
    end Apply;
    function Generic_Pointer is new Apply (Integers.To_Pointer);
    function Default_Pointer is new Apply;
    function Allocate (Size : Long_Integer) return System.Address;
    pragma Import (C, Allocate, "malloc");
    procedure Release (Value : System.Address);
    pragma Import (C, Release, "free");
    procedure Check (Value : Boolean) is
    begin
        if not Value then raise Program_Error; end if;
    end Check;
    procedure Run is
        X : aliased Integer := 42;
        P : Integers.Object_Pointer := Pointer (X'Address);
        type Derived_Pointer is new Integers.Object_Pointer;
        use type Derived_Pointer;
        Derived : Derived_Pointer := To_Pointer (X'Address);
        Foreign : System.Address;
        Q : Other_Integers.Object_Pointer := Other_Integers.To_Pointer (X'Address);
        L : Address_Views.Integers.Object_Pointer := Address_Views.Integers.To_Pointer (X'Address);
        type Holder is limited record
            Cell : aliased Integer := 3;
        end record;
        Box : aliased Holder;
        package Holders is new Views (Holder);
        use type Holders.Object_Pointer;
        H : Holders.Object_Pointer := Holders.To_Pointer (Box'Address);
        type Elements is array (1 .. 3) of aliased Integer;
        Data : aliased Elements := (11, 22, 33);
        package Arrays is new Views (Elements);
        use type Arrays.Object_Pointer;
        A : Arrays.Object_Pointer := Arrays.To_Pointer (Data'Address);
        subtype Text is String (3 .. 5);
        Characters : aliased Text := "ABC";
        package Strings is new Views (Text);
        S : Strings.Object_Pointer := Strings.To_Pointer (Characters'Address);
        type Matrix is array (1 .. 2, 4 .. 5) of Integer;
        Grid : aliased Matrix := ((1, 2), (3, 4));
        package Matrices is new Views (Matrix);
        M : Matrices.Object_Pointer := Matrices.To_Pointer (Grid'Address);
        type Bits is array (1 .. 9) of Boolean;
        pragma Pack (Bits);
        Flags : aliased Bits := (others => False);
        package Packed is new Views (Bits);
        B : Packed.Object_Pointer := Packed.To_Pointer (Flags'Address);
        Opaque : aliased Address_Views.Hidden := Address_Views.Create (17);
        package Private_Views is new Views (Address_Views.Hidden);
        Private_Pointer : Private_Views.Object_Pointer := Private_Views.To_Pointer (Opaque'Address);
        Calls : Natural := 0;
        Bad : Boolean := False;
        function Index return Positive is
        begin
            Calls := Calls + 1;
            return 2;
        end Index;
        function Pick return Integers.Object_Pointer is
        begin
            Calls := Calls + 1;
            return P;
        end Pick;
        function Place return System.Address is
        begin
            Calls := Calls + 1;
            return X'Address;
        end Place;
        generic
            type Item is limited private;
        package Nested is
            package Accesses is new Views (Item);
        end Nested;
        package Nested_Ints is new Nested (Integer);
        R : Nested_Ints.Accesses.Object_Pointer;
        Live : Integer := 0;
        Finalized : Integer := 0;
        type Guard is new Ada.Finalization.Limited_Controlled with record
            Value : Integer := 1;
        end record;
        overriding procedure Initialize (Object : in out Guard);
        overriding procedure Finalize (Object : in out Guard);
        overriding procedure Initialize (Object : in out Guard) is
        begin
            Live := Live + 1;
        end Initialize;
        overriding procedure Finalize (Object : in out Guard) is
        begin
            Live := Live - 1;
            Finalized := Finalized + 1;
        end Finalize;
        package Guards is new Views (Guard);
        use type Guards.Object_Pointer;
        Saved : Guards.Object_Pointer;
        type Owner is access Guard;
        procedure Free is new Ada.Unchecked_Deallocation (Guard, Owner);
        Owned : Owner;
        procedure Free_Instance is new Ada.Unchecked_Deallocation (Guard, Guards.Object_Pointer);
    begin
        Check (Derived = Derived_Pointer (P) and To_Address (Derived) = X'Address);
        Foreign := Allocate (Integer'Size / System.Storage_Unit);
        if Foreign = System.Null_Address then raise Storage_Error; end if;
        declare
            package Foreign_Views is new Views (Integer);
            F : Foreign_Views.Object_Pointer := Foreign_Views.To_Pointer (Foreign);
        begin
            F.all := 321;
        end;
        Check (Integers.To_Pointer (Foreign).all = 321);
        Release (Foreign);
        Check (P = X'Unchecked_Access and P.all = 42 and Address (P) = X'Address);
        Check (L = X'Unchecked_Access and Address_Views.Integers.To_Address (L) = X'Address);
        Check (Integers.Object_Pointer (Q) = P);
        P.all := 5;
        Check (X = 5 and Q.all = 5 and L.all = 5);
        Check (Pointer (System.Null_Address) = null and Address (null) = System.Null_Address);
        Check (Generic_Pointer (X'Address) = P and Default_Pointer (X'Address) = P);
        P := Integers.To_Pointer (Value => Place);
        Check (P.all = 5 and Calls = 1);
        Check (Integers.To_Address (Value => Pick) = X'Address and Calls = 2);
        P := Integers.To_Pointer (Data (Index)'Address);
        Check (P.all = 22 and Calls = 3);
        P.all := 99;
        Check (Data (2) = 99);
        P := Integers.To_Pointer (Box.Cell'Address);
        P.all := 7;
        Check (Box.Cell = 7 and H = Box'Unchecked_Access and H.Cell = 7);
        A.all (3) := 66;
        Check (Data (3) = 66 and A = Data'Unchecked_Access);
        Check (Arrays.To_Address (A) = Data'Address and A.all'First = 1);
        S.all (4) := 'Z';
        Check (Characters = "AZC" and S.all'First = 3);
        M.all (2, 5) := 9;
        Check (Grid (2, 5) = 9 and M.all'First (2) = 4);
        B.all (9) := True;
        Check (Flags (9) and Packed.To_Address (B) = Flags'Address);
        Check (Address_Views.Value (Private_Pointer.all) = 17);
        Check (Private_Views.To_Address (Private_Pointer) = Opaque'Address);
        R := Nested_Ints.Accesses.To_Pointer (X'Address);
        R.all := 8;
        Check (X = 8 and Nested_Ints.Accesses.To_Address (R) = X'Address);
        begin
            P := Integers.To_Pointer (System.Storage_Elements."+" (X'Address, 1));
        exception
            when Program_Error => Bad := True;
        end;
        Check (Bad);
        Bad := False;
        P := null;
        begin
            X := P.all;
        exception
            when Constraint_Error => Bad := True;
        end;
        Check (Bad);
        declare
            Local : aliased Guard;
        begin
            Saved := Guards.To_Pointer (Local'Address);
            Saved.Value := 77;
            Check (Live = 1 and Local.Value = 77 and Guards.To_Address (Saved) = Local'Address);
        end;
        Check (Live = 0 and Finalized = 1 and Saved /= null);
        Saved := null; -- No dereference after the designated object's lifetime.
        Owned := new Guard;
        Check (Live = 1);
        declare
            package Temporary is new Views (Guard);
            Borrowed : Temporary.Object_Pointer := Temporary.To_Pointer (Owned.all'Address);
        begin
            Borrowed.Value := 88;
            Check (Owned.Value = 88);
        end;
        Check (Live = 1 and Finalized = 1 and Owned.Value = 88);
        Free (Owned);
        Check (Live = 0 and Finalized = 2);
        Saved := new Guard;
        Check (Live = 1 and Guards.To_Pointer (Guards.To_Address (Saved)) = Saved);
        Free_Instance (Saved);
        Check (Saved = null and Live = 0 and Finalized = 3);
    end Run;
    protected Results is
        procedure Succeed;
        function Count return Natural;
    private
        Completed : Natural := 0;
    end Results;
    protected body Results is
        procedure Succeed is
        begin
            Completed := Completed + 1;
        end Succeed;
        function Count return Natural is
        begin
            return Completed;
        end Count;
    end Results;
    task type Worker;
    task body Worker is
    begin
        Run;
        Results.Succeed;
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Ada.Text_IO.Put_Line ("address conversions ok");
end AddressConversions;
