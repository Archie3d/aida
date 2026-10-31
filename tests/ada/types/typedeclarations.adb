with Ada.Text_IO;
procedure TypeDeclarations is
    S : String (1 .. 20) := (others => ' ');
    type Node;
    type Link is access Node;
    type Node is record
        Value : Integer;
    end record;
    package Hidden is
        type Item is private;
        function Make return Item;
    private
        type Item is record
            Value : Integer;
        end record;
    end Hidden;
    package body Hidden is
        function Make return Item is
        begin
            return (Value => 7);
        end Make;
    end Hidden;
    Item : Hidden.Item := Hidden.Make;
    Value : Integer := 42;
begin
    declare
        type S is record
            Field : Integer;
        end record;
        Item : S := (Field => Value);
        subtype Value is Integer range 1 .. 100;
        Result : Value := Item.Field;
    begin
        Ada.Text_IO.Put_Line (Integer'Image (Result));
    end;
    if S (1) /= ' ' then
        raise Program_Error;
    end if;
end TypeDeclarations;
