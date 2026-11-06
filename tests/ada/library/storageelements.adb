with Ada.Text_IO;
with System.Storage_Elements;
procedure StorageElements is
    use System.Storage_Elements;
    use type System.Address;
    package SE renames System.Storage_Elements;
    function Add (Left : System.Address; Right : Storage_Offset) return System.Address renames SE."+";
    function Offset_Add (Left : Storage_Offset; Right : System.Address) return System.Address renames SE."+";
    function Subtract (Left : System.Address; Right : Storage_Offset) return System.Address renames SE."-";
    function Difference (Left, Right : System.Address) return Storage_Offset renames SE."-";
    function Remainder (Left : System.Address; Right : Storage_Offset) return Storage_Offset renames SE."mod";
    generic
        with function Operation (Left : System.Address; Right : Storage_Offset) return System.Address is SE."+";
    function Apply (Left : System.Address; Right : Storage_Offset) return System.Address;
    function Apply (Left : System.Address; Right : Storage_Offset) return System.Address is
    begin
        return Operation (Left, Right);
    end Apply;
    function Generic_Add is new Apply (SE."+");
    function Default_Add is new Apply;
    function Generic_Subtract is new Apply (SE."-");
    function Fill (Destination : System.Address; Value : Integer; Count : Long_Integer) return System.Address;
    pragma Import (C, Fill, "memset");
    function Copy (Destination, Source : System.Address; Count : Long_Integer) return System.Address;
    pragma Import (C, Copy, "memcpy");
    procedure Check (Value : Boolean) is
    begin
        if not Value then raise Program_Error; end if;
    end Check;
    procedure Run is
        Data : aliased Storage_Array (-3 .. 4) := (others => 0);
        Other : Storage_Array (11 .. 18) := (others => 0);
        Empty : Storage_Array (1 .. 0);
        Edge : Storage_Array (Storage_Offset'First .. Storage_Offset'First + 1) := (1, 2);
        type Byte_Pointer is access all Storage_Element;
        Element : Byte_Pointer := Data (-2)'Access;
        A : constant System.Address := Data (-3)'Address;
        Max : constant Integer_Address := Integer_Address'Last;
        High : constant Integer_Address := 2 ** (System.Word_Size - 1);
        P : System.Address;
        Bad : Boolean := False;
        Captured : Integer_Address;
        Dynamic : Integer := -1;
        Count : Storage_Count := 0;
        type Octets is array (Integer range <>) of aliased Storage_Element;
        for Octets'Component_Size use System.Storage_Unit;
        type Derived is new Octets;
        Bytes : Octets (1 .. 2) := (10, 20);
        type Array_Pointer is access all Storage_Array (-3 .. 4);
        View : Array_Pointer := Data'Access;
        Null_View : Array_Pointer;
        Calls : Natural := 0;
        Fail : Boolean := False;
        function Array_Value return Storage_Array is
        begin
            Calls := Calls + 1;
            if Fail then raise Program_Error; end if;
            return Data;
        end Array_Value;
    begin
        Check (Storage_Offset'First = System.Min_Int and Storage_Offset'Last = System.Max_Int);
        Check (Storage_Count'First = 0 and Storage_Count'Last = Storage_Offset'Last);
        Check (Storage_Element'Modulus = 256 and Storage_Element'Size = System.Storage_Unit);
        Check (Storage_Array'Component_Size = System.Storage_Unit);
        Check (Data'Component_Size = 8 and Derived'Component_Size = 8);
        Check (Array_Value'Component_Size = 8 and Calls = 1);
        Check (View'Component_Size = 8);
        begin
            Count := Storage_Count (Null_View'Component_Size);
        exception
            when Constraint_Error => Bad := True;
        end;
        Check (Bad);
        Bad := False;
        Fail := True;
        begin
            Count := Storage_Count (Array_Value'Component_Size);
        exception
            when Program_Error => Bad := True;
        end;
        Check (Bad and Calls = 2);
        Bad := False;
        Check (Data'Size = 64 and Empty'Length = 0 and Edge'Length = 2);
        Check (Data (-2)'Address - A = 1 and Data (4)'Address - A = 7);
        Check (Data (-1 .. 1)'Length = 3 and Edge (Storage_Offset'First + 1) = 2);
        Check (To_Integer (System.Null_Address) = 0 and To_Address (0) = System.Null_Address);
        Check (To_Address (To_Integer (A)) = A);
        Check (To_Integer (To_Address (Max)) = Max and To_Integer (To_Address (High)) = High);
        Check (To_Address (Max) > To_Address (High));
        Check (To_Integer (Add (To_Address (Max), 1)) = 0);
        Check (To_Integer (Subtract (System.Null_Address, 1)) = Max);
        Check (To_Integer (Add (System.Null_Address, -1)) = Max);
        Check (To_Integer (Add (System.Null_Address, Storage_Offset'First)) = High);
        Check (To_Integer (Subtract (System.Null_Address, Storage_Offset'First)) = High);
        Check (Difference (To_Address (High), System.Null_Address) = Storage_Offset'First);
        Check (Difference (System.Null_Address, To_Address (High)) = Storage_Offset'First);
        Check (Difference (System.Null_Address, To_Address (Max)) = 1);
        Check (Difference (To_Address (Max), System.Null_Address) = -1);
        Check (Offset_Add (2, A) = Data (-1)'Address);
        Check (Generic_Add (A, 3) = Data (0)'Address and Default_Add (A, 4) = Data (1)'Address);
        Check (Generic_Subtract (A + 2, 2) = A);
        Check (SE."+" (Right => A, Left => 3) = Data (0)'Address);
        Check (SE."-" (Right => A, Left => A + 5) = 5);
        Check (Remainder (To_Address (17), 5) = 2 and To_Address (17) mod (-5) = -3);
        Check (To_Address (Max) mod Storage_Offset'First = -1);
        Check (To_Address (High) mod Storage_Offset'First = 0);
        Check (To_Address (Max) mod Storage_Offset'Last = 1);
        Check (A mod 1 = 0 and A mod (-1) = 0);
        P := Fill (A, 255, 8);
        Check (P = A and Data = (Data'Range => 255));
        Check (Element.all = 255);
        Element.all := 42;
        Check (Data (-2) = 42 and Element.all'Address = A + 1);
        Data (0 .. 1) := Storage_Array (Bytes);
        Check (Data (0) = 10 and Data (1) = 20);
        P := Fill (Data (1 .. 2)'Address, 7, 2);
        Check (P = A + 4 and Data (1) = 7 and Data (2) = 7 and Data (0) = 10);
        P := Data (5 .. 4)'Address;
        Check (To_Address (To_Integer (P)) = P);
        P := Copy (Other (11)'Address, A, 8);
        Check (Other = Data and P = Other (11)'Address);
        Check (A mod 8 = Storage_Offset (To_Integer (A) mod 8));
        begin
            P := To_Address (1);
            Count := P mod 0;
        exception
            when Program_Error => Bad := True;
        end;
        Check (Bad);
        Bad := False;
        begin
            Count := Storage_Count (Dynamic);
        exception
            when Constraint_Error => Bad := True;
        end;
        Check (Bad);
        declare
            Local : aliased Storage_Element := 3;
        begin
            Captured := To_Integer (Local'Address);
            Check (To_Address (Captured) = Local'Address);
        end;
        Check (To_Integer (To_Address (Captured)) = Captured);
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
    Ada.Text_IO.Put_Line ("storage elements ok");
end StorageElements;
