with Ada.Text_IO; use Ada.Text_IO;
with System;
with Alignment_Data;
procedure AlignmentClauses is
    function Aligned (Address : System.Address; Alignment : Long_Integer) return Integer;
    pragma Import (C, Aligned, "__test_aligned");
    type Byte is range 0 .. 255;
    for Byte'Size use 8;
    for Byte'Alignment use 1;
    type Cell is record
        Value : Byte;
    end record;
    for Cell'Alignment use 8;
    subtype Cell_View is Cell;
    type Derived_Cell is new Cell;
    for Derived_Cell'Alignment use 8;
    type Cells is array (1 .. 3) of Cell;
    type Octets is array (1 .. 8) of Byte;
    for Octets'Alignment use 8;
    type Packet is record
        Head : Byte;
        Data : Cell;
        Tail : Byte;
    end record;
    type No_Promise is range 0 .. 10;
    for No_Promise'Alignment use 0;
    type Empty_Record is null record;
    for Empty_Record'Alignment use 8;
    type Empty_Array is array (1 .. 0) of Cell;
    Empty_R : Empty_Record;
    Empty_A : Empty_Array;
    type Pointer is access Cell;
    A : Cells := ((Value => 11), (Value => 22), (Value => 33));
    B : Octets := (others => 7);
    P : Packet := (Head => 1, Data => (Value => 2), Tail => 3);
    D : Derived_Cell := (Value => 4);
    Heap : Pointer := new Cell'(Value => 5);
    Uninitialized : Integer;
    Nil : Pointer;
    Calls : Integer := 0;
    function Make return Cell is
    begin
        Calls := Calls + 1;
        return (Value => 6);
    end Make;
    procedure Check is
    begin
        if Aligned (P.Data'Address, 8) /= 1 or Aligned (A (2)'Address, 8) /= 1 then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
        Local : Cells := ((Value => 8), (Value => 9), (Value => 10));
    begin
        if Aligned (Local (2)'Address, 8) /= 1 or Local (2).Value /= 9 then
            raise Program_Error;
        end if;
    end Worker;
begin
    Check;
    if Uninitialized'Alignment /= Integer'Alignment then
        raise Program_Error;
    end if;
    begin
        if Nil.all'Alignment = 8 then
            raise Program_Error;
        end if;
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    if Aligned (Alignment_Data.Global (2)'Address, 8) /= 1
        or not Alignment_Data.Global (2).Value then
        raise Program_Error;
    end if;
    if Cell'Alignment /= 8 or Cell_View'Alignment /= 8 or Derived_Cell'Alignment /= 8
        or A'Alignment /= 8 or B'Alignment /= 8 or No_Promise'Alignment /= 0
        or Cell'Size /= 64 or Cells'Size /= 192 or Packet'Size /= 192
        or Aligned (Empty_R'Address, 8) /= 1 or Aligned (Empty_A'Address, 8) /= 1
        or Aligned (B'Address, 8) /= 1 or Aligned (D'Address, 8) /= 1
        or Aligned (Heap.all'Address, 8) /= 1 or Heap.all.Value /= 5
        or A (2).Value /= 22 or P.Data.Value /= 2 or P.Tail /= 3 then
        raise Program_Error;
    end if;
    if Make'Alignment /= 8 or Calls /= 1 then
        raise Program_Error;
    end if;
    declare
        W1, W2 : Worker;
    begin
        null;
    end;
    Put_Line ("alignment clauses preserve addresses and layouts");
end AlignmentClauses;
