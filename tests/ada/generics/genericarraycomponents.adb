with Ada.Text_IO;
procedure GenericArrayComponents is
    procedure Check (Length : Integer) is
        subtype Index is Integer range 1 .. Length;
        subtype Chunk is String (1 .. Length);
        subtype Alias_Chunk is Chunk;
        type Chunks is array (Index) of Alias_Chunk;
        generic
            type A is array (Index) of Chunk;
        function Copy (Value : A) return A;
        function Copy (Value : A) return A is
        begin
            return Value;
        end Copy;
        function Copy_Chunks is new Copy (Chunks);
        Value : Chunks := (others => (others => 'x'));
        type Record_Item (Count : Integer) is record
            Value : Integer;
        end record;
        subtype Item is Record_Item (Length);
        subtype Alias_Item is Item;
        type Items is array (Index) of Alias_Item;
        generic
            type A is array (Index) of Item;
        function Copy_Items (Value : A) return A;
        function Copy_Items (Value : A) return A is
        begin
            return Value;
        end Copy_Items;
        function Record_Copy is new Copy_Items (Items);
        Records : Items := (others => (Count => Length, Value => 7));
    begin
        if Copy_Chunks (Value) /= Value or Record_Copy (Records) /= Records then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check (3);
        Check (0);
    end Worker;
begin
    Check (2);
    Check (0);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("nonstatic array component constraints passed");
end GenericArrayComponents;
