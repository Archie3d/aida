with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure SharedScalars is
    type Byte is range -128 .. 127;
    for Byte'Size use 8;
    pragma Atomic (Byte);
    type Half is range -32768 .. 32767;
    for Half'Size use 16;
    pragma Volatile (Half);
    type Unsigned_Half is mod 65536;
    for Unsigned_Half'Size use 16;
    pragma Atomic (Unsigned_Half);
    type Shared_Integer is new Integer;
    pragma Atomic (Shared_Integer);
    subtype Small is Shared_Integer range 0 .. 10;
    type Derived is new Shared_Integer;
    b : Byte := -128;
    h : Half := -32768;
    u : Unsigned_Half := 65535;
    i : Integer := -2147483648;
    pragma Volatile (i);
    l : Long_Integer := Long_Integer'First;
    pragma Atomic (l);
    f : Float := -1.25;
    pragma Atomic (f);
    d : Long_Float := -2.5;
    pragma Volatile (d);
    interval : Duration := -1.5;
    pragma Atomic (interval);
    ch : Character := 'z';
    pragma Volatile (ch);
    flag : Boolean := True;
    pragma Atomic (flag);
    original : Integer := 7;
    alias : Integer renames original;
    secondAlias : Integer renames alias;
    pragma Atomic (original);
    typed : Shared_Integer := 4;
    inherited : Derived := 9;
    constrained : Small := 1;
    snapshot : constant Shared_Integer := 6;
    type Pointer is access Shared_Integer;
    p : Pointer := new Shared_Integer'(12);
    pragma Atomic (p);
    q : Pointer;
    pragma Volatile (q);
    type Atomic_Pointer is access Integer;
    pragma Atomic (Atomic_Pointer);
    type Pointer_Holder is access Atomic_Pointer;
    holder : Pointer_Holder := new Atomic_Pointer;
    procedure Free_Holder is new Ada.Unchecked_Deallocation (Atomic_Pointer, Pointer_Holder);
    procedure Free is new Ada.Unchecked_Deallocation (Shared_Integer, Pointer);
    procedure Change (value : in out Integer) is
    begin
        value := value + 1;
    end Change;
    procedure Fail (value : in out Integer) is
    begin
        value := 99;
        raise Constraint_Error;
    end Fail;
    procedure Overflow (value : out Shared_Integer) is
    begin
        value := 100;
    end Overflow;
    procedure Capture (padding : Integer; value : Shared_Integer) is
        function Nested return Shared_Integer is
        begin
            return value + Shared_Integer (padding);
        end Nested;
    begin
        if Nested /= 7 then
            raise Program_Error;
        end if;
    end Capture;
    generic
        value : in out Shared_Integer;
    procedure Bump;
    procedure Bump is
    begin
        value := value + 1;
    end Bump;
    procedure Bump_Typed is new Bump (typed);
    generic
        initial : Integer;
    package Model is
        type Value_Type is new Integer;
        pragma Volatile (Value_Type);
        value : Value_Type := Value_Type (initial);
    end Model;
    package First_Model is new Model (3);
    package Second_Model is new Model (5);
begin
    if b /= -128 or h /= -32768 or u /= 65535 or i /= -2147483648
        or l /= Long_Integer'First or f /= -1.25 or d /= -2.5 or interval /= -1.5
        or ch /= 'z' or not flag or inherited /= 9 or snapshot /= 6 or q /= null or holder.all /= null then
        raise Program_Error;
    end if;
    b := 127;
    h := 32767;
    u := 1;
    l := Long_Integer'Last;
    f := 0.0;
    d := 1.25;
    interval := 2.0;
    ch := 'A';
    flag := False;
    secondAlias := 8;
    Change (original);
    begin
        Fail (secondAlias);
    exception
        when Constraint_Error => null;
    end;
    begin
        Overflow (constrained);
    exception
        when Constraint_Error => null;
    end;
    Bump_Typed;
    First_Model.value := First_Model.Value_Type (8);
    if Integer (First_Model.value) /= 8 or Integer (Second_Model.value) /= 5 then
        raise Program_Error;
    end if;
    Capture (2, typed);
    p.all := p.all + 1;
    if b /= 127 or h /= 32767 or u /= 1 or l /= Long_Integer'Last
        or f /= 0.0 or d /= 1.25 or interval /= 2.0 or ch /= 'A' or flag
        or original /= 9 or constrained /= 1 or typed /= 5 or p.all /= 13 then
        raise Program_Error;
    end if;
    Free_Holder (holder);
    Free (p);
    if p /= null then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("shared scalars passed");
end SharedScalars;
