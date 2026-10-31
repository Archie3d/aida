with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure DiscriminantComponents is
    Captures : Integer := 0;
    function Lower return Positive is
    begin
        Captures := Captures + 1;
        return 2;
    end Lower;
    type Buffer (Length : Natural := 4) is record
        Text : String (Lower .. Length) := (others => 'x');
        Count : Natural := Length;
    end record;
    type Inner (D : Natural) is record
        Data : String (1 .. D) := (others => 'n');
    end record;
    type Outer (Size : Natural := 3) is record
        Part : Inner (Size);
    end record;
    type Inner_Ref is access Inner;
    type Allocating (D : Natural := 2) is record
        Value : Inner_Ref := new Inner (D + 1);
    end record;
    Allocated_One, Allocated_Two : Allocating;
    type Matrix is array (Integer range <>, Integer range <>) of Integer;
    type Grid (Rows, Columns : Natural := 2) is record
        Data : Matrix (1 .. Rows, 2 .. Columns) := (others => (others => 7));
    end record;
    Grouped_Captures : Natural := 0;
    function Grouped_Lower return Positive is
    begin
        Grouped_Captures := Grouped_Captures + 1;
        return Grouped_Captures;
    end Grouped_Lower;
    type Grouped (D : Natural := 3) is record
        Left, Right : String (Grouped_Lower .. D) := (others => 'g');
    end record;
    Grouped_Object : Grouped;
    N : Natural := 5;
    A : Buffer (N);
    B : Buffer;
    C : Buffer := (6, "abcde", 6);
    O : Outer;
    G : Grid (N, 4);
    type Ref is access Buffer;
    P : Ref := new Buffer (N);
    procedure Free is new Ada.Unchecked_Deallocation (Buffer, Ref);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    procedure Set (Target : in out Buffer; Source : Buffer) is
    begin
        Target := Source;
    end Set;
    type Callback is access procedure (Target : in out Buffer; Source : Buffer);
    Action : Callback := Set'Access;
    function Copy (X : Buffer) return Buffer is
    begin
        return X;
    end Copy;
    subtype Fixed_Buffer is Buffer (N);
    function Fixed_Copy (X : Buffer) return Fixed_Buffer is
    begin
        return X;
    end Fixed_Copy;
    function Make (Size : Natural) return Buffer is
    begin
        return (Size, (others => 'r'), Size);
    end Make;
    procedure Recurse (Depth : Natural) is
        X : Buffer (Depth);
    begin
        Check (X.Length = Depth and X.Text'Length = (Depth - 1));
        if Depth > 1 then Recurse (Depth - 1); end if;
        Check (X.Length = Depth);
    end Recurse;
begin
    Check (Grouped_Captures = 2 and Grouped_Object.Left = "ggg" and Grouped_Object.Right = "gg");
    Check (Captures = 1 and A.Text = "xxxx" and A.Count = 5 and B.Text = "xxx");
    Check (Allocated_One.Value.D = 3 and Allocated_Two.Value.Data = "nnn");
    Check (O.Part.D = 3 and O.Part.Data = "nnn");
    Check (G.Data'Length (1) = 5 and G.Data'Length (2) = 3 and G.Data (5, 4) = 7);
    Check (P.Text = "xxxx" and P.all'Constrained);
    Action (B, C);
    Check (B = C);
    B.Text (2) := 'z';
    Check (B.Text = "zbcde" and C.Text = "abcde" and B.Length = 6);
    B.Text (3 .. 4) := "qq";
    Check (B.Text = "zqqde");
    B := Copy (A);
    Check (B.Text = "xxxx" and B.Length = 5);
    B := Make (2);
    Check (B.Text = "r" and B.Text'First = 2 and B.Text'Last = 2);
    B := Make (0);
    Check (B.Text'Length = 0 and B.Text'First = 2 and B.Text'Last = 0);
    begin
        B := Fixed_Copy (C);
        raise Program_Error;
    exception
        when Constraint_Error => Check (B.Length = 0 and B.Text'Length = 0);
    end;
    begin
        Set (A, C);
        raise Program_Error;
    exception
        when Constraint_Error => Check (A.Length = 5 and A.Text = "xxxx");
    end;
    declare
        type Copies is array (1 .. 2) of Buffer;
        X, Y : Copies;
        type Derived is new Buffer;
        D : Derived;
    begin
        X (1) := C;
        Y := X;
        X (1).Text (2) := 'w';
        Check (Y (1).Text = "abcde" and X (1).Text = "wbcde");
        Check (D.Text = "xxx");
    end;
    Recurse (5);
    Free (P);
    Check (P = null and Captures = 1);
    Put_Line ("discriminant components ok");
end DiscriminantComponents;
