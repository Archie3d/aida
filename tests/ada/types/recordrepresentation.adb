with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Record_Layout; use Record_Layout;
with System;
procedure RecordRepresentation is
    function Offset (First, Second : System.Address) return Long_Integer;
    pragma Import (C, Offset, "__test_size_offset");
    function Raw (Address : System.Address; Bytes, Signed : Integer) return Long_Integer;
    pragma Import (C, Raw, "__test_size_value");
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    type Partial is record
        A : Integer := 1;
        B : Character := 'z';
        C : Integer := 3;
    end record;
    for Partial use record
        at mod 8;
        B at 0 range 8 .. 15;
    end record;
    type Compact is record
        Small : Integer range -128 .. 127 := -12;
        Medium : Integer range 0 .. 65535 := 60000;
        Tail : Character := 't';
    end record;
    for Compact'Size use 32;
    for Compact use record
        Small at 0 range 0 .. 7;
        Tail at 1 range 0 .. 7;
        Medium at 2 range 0 .. 15;
    end record;
    Small_Record : Compact;
    procedure Increase (Value : in out Integer) is
    begin
        Value := Value + 1;
    end Increase;
    subtype View is Frame;
    type Derived is new Frame;
    type Frames is array (1 .. 2) of Frame;
    type Link is access Frame;
    procedure Free is new Ada.Unchecked_Deallocation (Frame, Link);
    A : Frame;
    B : Frame := (False, True, Header => 5, Letter => 'a');
    C : Frame (False, False);
    Copy : View := A;
    D : Derived := (True, False, 7, 8, 9);
    Values : Frames := (others => <>);
    P : Partial;
    Heap : Link := new Frame'(B);
    Calls : Integer := 0;
    function Make return Frame is
    begin
        Calls := Calls + 1;
        return (True, False, 41, 42, 43);
    end Make;
    procedure Update (Object : in out Frame) is
    begin
        Object := (False, False, Header => 71, Wide => 72);
    end Update;
    generic
        type Item is private;
    procedure Swap (Left, Right : in out Item);
    procedure Swap (Left, Right : in out Item) is
        Temp : Item := Left;
    begin
        Left := Right;
        Right := Temp;
    end Swap;
    procedure Swap_Frames is new Swap (Frame);
    task type Worker;
    task body Worker is
        Local : Frame;
    begin
        for I in 1 .. 10 loop
            Local := (False, False, Header => I, Wide => 81);
            Check (Local.Wide = 81 and Local.Header = I);
            Check (Offset (Local'Address, Local.Header'Address) = 16);
        end loop;
    end Worker;
begin
    Check (Compact'Size = 32 and Small_Record.Small'Size = 8 and Small_Record.Medium'Size = 16);
    Check (Raw (Small_Record'Address, 1, 1) = -12);
    Check (Raw (Small_Record.Medium'Address, 2, 0) = 60000);
    Increase (Small_Record.Small);
    Check (Small_Record.Small = -11 and Small_Record.Tail = 't' and Small_Record.Medium = 60000);
    Check (Frame'Size = 192 and A.Header'Position = 16 and A.First'Position = 0);
    Check (A.First'First_Bit = 0 and A.First'Last_Bit = 31);
    Check (Offset (A'Address, A.Header'Address) = 16);
    Check (Raw (A'Address, 4, 1) = 11 and Raw (A.Second'Address, 4, 1) = 22);
    Check (Global.Header = 9 and Offset (Global'Address, Global.Header'Address) = 16);
    Check (Offset (Values (1)'Address, Values (2)'Address) = 24);
    Check (Offset (D'Address, D.Header'Address) = 16 and Frame (D).Second = 9);
    Check (P.B'Position = 1 and P.B'First_Bit = 0 and P.B'Last_Bit = 7);
    Check (P.A'Position = 4 and P.C'Position = 8 and Partial'Alignment = 8);
    Check (Make.Header'Position = 16 and Calls = 1);
    Swap_Frames (A, B);
    Check (A.Letter = 'a' and B.Second = 22 and Copy.First = 11);
    Update (B);
    Check (B.Wide = 72 and B.Header = 71);
    Check (Heap.Letter = 'a' and Offset (Heap.all'Address, Heap.Header'Address) = 16);
    Free (Heap);
    declare
        Captured : Frame := Make;
        procedure Change is
        begin
            Check (Offset (Captured'Address, Captured.Header'Address) = 16);
            Captured.Second := 99;
        end Change;
    begin
        Change;
        Check (Captured.Second = 99);
    end;
    declare
        W1, W2, W3, W4 : Worker;
    begin
        null;
    end;
    Put_Line ("record representation passed");
end RecordRepresentation;
