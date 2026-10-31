with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Limited_A;
with Limited_B;
procedure LimitedWith is
    use type Limited_B.Nested.Scalar;
    procedure Free_A is new Ada.Unchecked_Deallocation (Limited_A.Node, Limited_B.A_Access);
    procedure Free_B is new Ada.Unchecked_Deallocation (Limited_B.Node, Limited_A.B_Access);
    procedure Exercise is
        A : Limited_B.A_Access := new Limited_A.Node;
        B : Limited_A.B_Access := new Limited_B.Node;
        C : access Limited_B.Node := Limited_A.Create;
        W : Limited_A.B_Class_Access := new Limited_B.Node;
        Derived : Limited_A.Derived_Access := new Limited_B.Derived_Node;
        Alias_View : Limited_A.Incomplete_B;
        Renamed_Node : Limited_A.Renamed_B.Node;
        Renamed_Nested : Limited_A.Renamed_Nested.Data;
        Vector : Limited_A.Vector_View := (others => 5);
        Scalar : Limited_A.Scalar_Access := new Limited_B.Nested.Scalar'(7);
        Nested : Limited_A.Nested_Access := new Limited_B.Nested.Data;
    begin
        A.Link (B);
        if Scalar.all /= 7 or Nested.Value /= 9 or Renamed_Node.Read /= 2 or Renamed_Nested.Value /= 9 or Vector'Length /= 3 or Vector (3) /= 5 then raise Program_Error; end if;
        if B.Value /= 11 or A.Peer.Value /= 11 or Limited_A.Inspect (B.all) /= 11
            or C.Read /= 2 or W.Read /= 2 or Derived.Read /= 2 or Alias_View.Read /= 2 then
            raise Program_Error;
        end if;
        if A.Peer.Value'Size /= Integer'Size or A.Peer.Value'Position <= 0 then raise Program_Error; end if;
        B.Peer := A;
        if B.Peer.Value /= 1 then raise Program_Error; end if;
        Free_B (B);
        Free_A (A);
    end Exercise;
    protected Results is
        procedure Done;
        function Count return Integer;
    private
        Total : Integer := 0;
    end Results;
    protected body Results is
        procedure Done is
        begin
            Total := Total + 1;
        end Done;
        function Count return Integer is
        begin
            return Total;
        end Count;
    end Results;
begin
    Exercise;
    declare
        task type Worker;
        task body Worker is
        begin
            Exercise;
            Results.Done;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    if Results.Count /= 4 then raise Program_Error; end if;
    Put_Line ("limited with passed");
end LimitedWith;
