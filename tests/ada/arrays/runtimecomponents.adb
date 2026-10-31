with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure RuntimeComponents is
    Calls : Integer := 0;
    function Bound return Integer is
    begin
        Calls := Calls + 1;
        return Calls + 1;
    end Bound;
    N : Integer := 3;
    subtype Text is String (2 .. N);
    type Pair is record
        Lead : Integer := 7;
        A, B : String (1 .. Bound) := (others => 'a');
        C : Text := (others => 'c');
        Tail : Long_Integer := 99;
    end record;
    type Pairs is array (1 .. 2) of Pair;
    type Rows is array (1 .. 2) of Text;
    type Nested is record
        Value : Pair;
        Items : Rows := (others => (others => 'r'));
        Last : Integer := 42;
    end record;
    P : Pair;
    Q : Pair := P;
    Items : Pairs;
    R : Rows := (others => (others => 'r'));
    H : Nested;
    type Choice (Kind : Boolean) is record
        Head : Integer := 7;
        case Kind is
            when True =>
                Data : Text := "cc";
                Last : Long_Integer := 99;
            when False =>
                Other : String (1 .. N + 1) := (others => 'z');
        end case;
    end record;
    Yes : Choice (True);
    No : Choice (False);
    type Pointer is access Pair;
    procedure Free is new Ada.Unchecked_Deallocation (Pair, Pointer);
    Heap : Pointer := new Pair;
    procedure Check (X : Pair; Y : in out Pair) is
        Copy : Pair := X;
    begin
        if Copy /= X or Copy.A'Length /= 2 or Copy.B'Length /= 3 then raise Program_Error; end if;
        Y := X;
    end Check;
    function Make return Pair is
    begin
        return P;
    end Make;
    procedure Recurse (Depth : Integer) is
        type Local is record
            Data : String (1 .. Depth) := (others => 'r');
            Last : Long_Integer := 42;
        end record;
        X : Local;
        function Echo return Local is
        begin
            return X;
        end Echo;
    begin
        if Depth > 1 then Recurse (Depth - 1); end if;
        declare
            Y : Local := Echo;
        begin
            if Y /= X or Y.Data'Length /= Depth or Y.Last /= 42 then raise Program_Error; end if;
        end;
    end Recurse;
begin
    Recurse (4);
    N := 100;
    if Calls /= 2 or P.Lead /= 7 or P.Tail /= 99 or P.A /= "aa" or P.B /= "aaa"
        or P.C /= "cc" or Items (2).Tail /= 99 or R (2) /= "rr"
        or H.Value.B /= "aaa" or H.Items (2) /= "rr" or H.Last /= 42 then
        raise Program_Error;
    end if;
    if Yes.Data /= "cc" or Yes.Last /= 99 or No.Other /= "zzzz" or Yes.Head /= 7 then
        raise Program_Error;
    end if;
    Check (Make, Q);
    if P /= Q or Heap.all /= P or Text'Size /= 16 or P.C'Size /= 16
        or Pair'Size /= 192 or P'Size /= 192 or Nested'Size /= 256 or Rows'Size /= 32 then raise Program_Error; end if;
    P.A (2) := 'x';
    Items (2) := P;
    if Items (2).A /= "ax" or Items (1).A /= "aa" then raise Program_Error; end if;
    Heap.all := P;
    if Heap.A /= "ax" or Heap.Tail /= 99 then raise Program_Error; end if;
    Free (Heap);
    Ada.Text_IO.Put_Line ("runtime components ok");
end RuntimeComponents;
