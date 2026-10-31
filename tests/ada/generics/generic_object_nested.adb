with Ada.Text_IO;
procedure Generic_Object_Nested is
    generic
        type Item is private;
        Value : Item;
        Other : Item := Value;
    function Copy return Item;
    function Copy return Item is
    begin
        return Other;
    end Copy;

    generic
        Input : Integer;
        Output : in out Integer;
    package Outer is
        function Read is new Copy (Integer, Input);
        procedure Set;
    end Outer;
    package body Outer is
        generic
            Value : in out Integer;
        procedure Store;
        procedure Store is
        begin
            Value := Input;
        end Store;
        procedure Store_Output is new Store (Output);
        procedure Set is
        begin
            Store_Output;
        end Set;
    end Outer;
    A : Integer := 3;
    B : Integer := 9;
    X : Integer := 0;
    Y : Integer := 0;
    package First is new Outer (A, X);
    package Second is new Outer (B, Y);
    function Direct is new Copy (Integer, B);
begin
    A := 20;
    B := 30;
    First.Set;
    Second.Set;
    if First.Read /= 3 or Second.Read /= 9 or Direct /= 9 or X /= 3 or Y /= 9 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("nested generic objects and instance-owned defaults");
end Generic_Object_Nested;
