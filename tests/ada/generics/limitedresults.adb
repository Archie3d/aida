with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Return_Model; use Return_Model;
procedure LimitedResults is
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    procedure Check_Cell (Object : in out Cell; Value : Integer) is
    begin
        Check (Object.Value = Value and Object.Self = Object.Value'Unchecked_Access);
    end Check_Cell;
    function Forward_Array (Count : Natural) return Cells is
    begin
        return Many (Count);
    end Forward_Array;
    function Initialized return Cell is
    begin
        return Result : Cell := Make (8) do
            Result.Value := Result.Value + 1;
        end return;
    end Initialized;
    function Initialized_Array return Cells is
    begin
        return Result : Cells := Many (2) do
            Result (3).Value := 11;
        end return;
    end Initialized_Array;
    function Aggregate_Array return Cells is
    begin
        return (1 => Make (1), 2 => Make (2));
    end Aggregate_Array;
    subtype Cell_Alias is Cell;
    type Cell_Matrix is array (1 .. 2, 1 .. 2) of Cell;
    generic
        type Item is limited private;
    function Default_Result return Item;
    function Default_Result return Item is
    begin
        return Result : Item;
    end Default_Result;
    function Default_Cell is new Default_Result (Cell);
    type Cells_Link is access Cells;
    procedure Free_Cells is new Ada.Unchecked_Deallocation (Cells, Cells_Link);
    function "+" (Left, Right : Integer) return Cell is
    begin
        return Make (Left + Right);
    end "+";
    function "+" (Value : Integer) return Cell is
    begin
        return Make (Value);
    end "+";
    function Operator_Result return Cell is
    begin
        return 2 + 3;
    end Operator_Result;
    type Cell_Link is access Cell;
    procedure Free_Cell is new Ada.Unchecked_Deallocation (Cell, Cell_Link);
    type Buffer_Link is access Buffer;
    procedure Free_Buffer is new Ada.Unchecked_Deallocation (Buffer, Buffer_Link);
    type Limited_Tag is tagged limited record
        Value : aliased Integer := 0;
        Self : Link;
    end record;
    function Tagged_Result return Limited_Tag is
    begin
        return Result : Limited_Tag do
            Result.Value := 4;
            Result.Self := Result.Value'Unchecked_Access;
        end return;
    end Tagged_Result;
    function Aggregate_Wide return Limited_Tag'Class is
    begin
        return Limited_Tag'(Value => 11, Self => null);
    end Aggregate_Wide;
    function Wide_Result return Limited_Tag'Class is
    begin
        return Result : Limited_Tag'Class := Tagged_Result do
            Result.Value := 10;
        end return;
    end Wide_Result;
    task type Worker;
    task body Worker is
        Local : Cell := Forward (7);
        Values : Cells := Many (2);
        Bytes : Buffer := Make_Buffer (3);
    begin
        Check_Cell (Local, 7);
        Check_Cell (Values (3), 3);
        Check (Bytes.Text = "zzz" and Bytes.Item.Self = Bytes.Item.Value'Unchecked_Access);
    end Worker;
    Object : Cell := Forward (7);
    Alias_Object : Cell_Alias := Make (15);
    Binary_Object : Cell := Operator_Result;
    Unary_Object : Cell := +6;
    Generic_Object : Cell := Default_Cell;
    Aggregate_Values : Cells := Aggregate_Array;
    Matrix : Cell_Matrix := ((Make (1), Make (2)), (Make (3), Make (4)));
    Heap_Values : Cells_Link := new Cells'(Many (2));
    Other : Cell := Initialized;
    Values : Cells := Forward_Array (3);
    Items : Pair := Make_Pair;
    Aggregate : Cells := (1 => Make (1), 2 => Make (2));
    Initialized_Values : Cells := Initialized_Array;
    Bytes : Buffer := Make_Buffer (4);
    Default_Bytes : Buffer;
    Heap : Cell_Link := new Cell'(Forward (12));
    Heap_Bytes : Buffer_Link := new Buffer'(Make_Buffer (3));
    Tagged_Object : Limited_Tag'Class := Wide_Result;
    Aggregate_Tagged : Limited_Tag'Class := Aggregate_Wide;
begin
    Check_Cell (Object, 7);
    Check_Cell (Alias_Object, 15);
    Check_Cell (Binary_Object, 5);
    Check_Cell (Unary_Object, 6);
    Check (Generic_Object.Value = 0 and Generic_Object.Self = null);
    Check_Cell (Aggregate_Values (1), 1);
    Check_Cell (Aggregate_Values (2), 2);
    Check_Cell (Matrix (2, 2), 4);
    Check_Cell (Heap_Values (3), 3);
    Check_Cell (Other, 9);
    Check_Cell (Values (3), 3);
    Check_Cell (Values (5), 5);
    Check_Cell (Items.First, 5);
    Check_Cell (Items.Second, 6);
    Check_Cell (Aggregate (1), 1);
    Check_Cell (Aggregate (2), 2);
    Check_Cell (Initialized_Values (3), 11);
    Check_Cell (Heap.all, 12);
    Check (Bytes.Length = 4 and Bytes.Text = "zzzz" and Bytes'Constrained);
    Check (Default_Bytes.Length = 2 and Default_Bytes'Constrained);
    Check (Heap_Bytes.Text = "zzz");
    Check (Aggregate_Tagged.Value = 11);
    Check (Tagged_Object.Value = 10 and Tagged_Object.Self = Tagged_Object.Value'Unchecked_Access);
    begin
        declare
            Bad : Buffer (2) := Make_Buffer (3);
        begin
            Check (False);
        end;
    exception
        when Constraint_Error => null;
    end;
    begin
        declare
            Bad : Cells (1 .. 1) := Many (2);
        begin
            Check (False);
        end;
    exception
        when Constraint_Error => null;
    end;
    Free_Cells (Heap_Values);
    Free_Cell (Heap);
    Free_Buffer (Heap_Bytes);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Put_Line ("limited results passed");
end LimitedResults;
