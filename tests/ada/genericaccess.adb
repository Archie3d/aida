with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure GenericAccess is
    type Item is record
        Value : Integer := 0;
    end record;
    type Link is access Item;
    type General_Link is access all Item;
    type Constant_Link is access constant Item;
    subtype Nonnull_Link is not null General_Link;
    type Action is access procedure (Value : in out Integer);
    type Query is access function (Value : Integer) return Integer;
    procedure Increment (Value : in out Integer) is
    begin
        Value := Value + 1;
    end Increment;
    function Double (Value : Integer) return Integer is
    begin
        return Value * 2;
    end Double;
    generic
        type Element is private;
        type Pointer is access Element;
    package Pool is
        function Make (Value : Element) return Pointer;
        procedure Free is new Ada.Unchecked_Deallocation (Element, Pointer);
    end Pool;
    package body Pool is
        function Make (Value : Element) return Pointer is
        begin
            return new Element'(Value);
        end Make;
    end Pool;
    generic
        type Pointer is access all Item;
    procedure Set (Value : Pointer);
    procedure Set (Value : Pointer) is
    begin
        Value.Value := Value.Value + 10;
    end Set;
    generic
        type Pointer is access constant Item;
    function Read (Value : Pointer) return Integer;
    function Read (Value : Pointer) return Integer is
    begin
        return Value.Value;
    end Read;
    generic
        type Pointer is not null access all Item;
    procedure Nonnull_Set (Value : Pointer);
    procedure Nonnull_Set (Value : Pointer) is
    begin
        Value.Value := Value.Value + 1;
    end Nonnull_Set;
    generic
        type Callback is access procedure (Value : in out Integer);
        type Mapper is access function (Value : Integer) return Integer;
    procedure Invoke (Call : Callback; Map : Mapper; Value : in out Integer);
    procedure Invoke (Call : Callback; Map : Mapper; Value : in out Integer) is
    begin
        Call (Value);
        Value := Map (Value);
    end Invoke;
    package Allocator is new Pool (Item, Link);
    package General_Allocator is new Pool (Item, General_Link);
    procedure Update is new Set (General_Link);
    function Get is new Read (Constant_Link);
    procedure Update_Nonnull is new Nonnull_Set (Nonnull_Link);
    procedure Run_Callbacks is new Invoke (Action, Query);
    procedure Check is
        P : Link := Allocator.Make ((Value => 8));
        G : General_Link := General_Allocator.Make ((Value => 9));
        Number : Integer := 4;
    begin
        Update (G);
        Update_Nonnull (Nonnull_Link (G));
        if Get (Constant_Link (G)) /= 20 or P.Value /= 8 or G.Value /= 20 then
            raise Program_Error;
        end if;
        Run_Callbacks (Increment'Access, Double'Access, Number);
        if Number /= 10 then
            raise Program_Error;
        end if;
        Allocator.Free (P);
        General_Allocator.Free (G);
        if P /= null or G /= null then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("generic access types passed");
end GenericAccess;
