with Ada.Text_IO;
procedure Generic_Objects is
    Calls : Integer := 0;
    Seed : Integer := 7;
    function Next_Value return Integer is
    begin
        Calls := Calls + 1;
        return Seed;
    end Next_Value;

    generic
        Value : in Integer;
        Extra : Integer := Value + Next_Value;
    package Snapshot is
        function Read return Integer;
    end Snapshot;
    package body Snapshot is
        function Read return Integer is
        begin
            return Value + Extra;
        end Read;
    end Snapshot;

    generic
        Value : in out Integer;
        Amount : Integer := 1;
    procedure Add;
    procedure Add is
    begin
        Value := Value + Amount;
    end Add;

    procedure Run (Initial, Depth : Integer) is
        Local : Integer := Initial;
        package First is new Snapshot (Local);
        package Second is new Snapshot (Extra => Local, Value => Next_Value);
        procedure Increment is new Add (Local);
        -- A shadow at the instance site must not change a default's meaning.
        function Next_Value return Integer is
        begin
            return 1000;
        end Next_Value;
        package Third is new Snapshot (Local);
    begin
        Local := Local + 10;
        Increment;
        if Local /= Initial + 11 or First.Read /= 2 * Initial + Seed
            or Second.Read /= Seed + Initial or Third.Read /= First.Read then
            raise Program_Error;
        end if;
        if Depth > 0 then
            Run (Initial + 20, Depth - 1);
        end if;
        if First.Read /= 2 * Initial + Seed then
            raise Program_Error;
        end if;
    end Run;

    type Numbers is array (1 .. 3) of Integer;
    Items : Numbers := (10, 20, 30);
    Index : Integer := 1;
    procedure Add_Item is new Add (Items (Index), 5);
    type Pair is record
        Value : Integer;
    end record;
    Item : Pair := (Value => 4);
    procedure Add_Field is new Add (Item.Value);

    generic
        Capacity : Positive;
    package Dynamic is
        subtype Count is Natural range 0 .. Capacity;
        type Vector is array (1 .. Capacity) of Integer;
        Values : Vector := (others => Capacity);
        Last : Count := Capacity;
    end Dynamic;
    package Sized is new Dynamic (Seed);

    generic
        Value : Float;
        Flag : Boolean;
    function Real_Value return Float;
    function Real_Value return Float is
    begin
        if Flag then
            return Value;
        end if;
        return 0.0;
    end Real_Value;
    Real_Seed : Float := 2.5;
    Flag_Seed : Boolean := True;
    function Saved_Real is new Real_Value (Real_Seed, Flag_Seed);
begin
    Run (3, 2);
    Run (9, 0);
    if Calls /= 12 then
        raise Program_Error;
    end if;
    Index := 2;
    Add_Item;
    Add_Field;
    if Items (1) /= 15 or Items (2) /= 20 or Item.Value /= 5 then
        raise Program_Error;
    end if;
    Seed := 12;
    Real_Seed := 9.0;
    Flag_Seed := False;
    if Sized.Values'Length /= 7 or Sized.Values (7) /= 7 or Sized.Last /= 7
        or Saved_Real /= 2.5 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("generic objects: snapshots, defaults, references, and dynamic bounds");
end Generic_Objects;
