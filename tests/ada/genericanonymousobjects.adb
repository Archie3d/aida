with Ada.Text_IO;
procedure GenericAnonymousObjects is
    procedure Check is
        A : aliased Integer := 3;
        B : aliased Integer := 7;
        P : access Integer := A'Access;
        Q : access Integer := B'Access;
        Fixed : aliased constant Integer := 19;
        Read_Only : access constant Integer := Fixed'Access;
        generic
            Value : access Integer;
            Alias_1 : in out access Integer;
            Alias_2 : in out access Integer;
            Constant_Value : access constant Integer;
        package Model is
            function Read return Integer;
            procedure Update;
        end Model;
        package body Model is
            function Read return Integer is
            begin
                return Value.all + Constant_Value.all;
            end Read;
            procedure Update is
            begin
                Value.all := Value.all + 1;
                Alias_1 := Q;
                Alias_2.all := Alias_2.all + 2;
            end Update;
        end Model;
        package Instance is new Model (P, P, P, Read_Only);
        generic
            Value : not null access Integer := new Integer'(29);
        function Read_Default return Integer;
        function Read_Default return Integer is
        begin
            return Value.all;
        end Read_Default;
        function Get_Default is new Read_Default;
        generic
            Value : in out not null access Integer;
        procedure Increment;
        procedure Increment is
        begin
            Value.all := Value.all + 1;
        end Increment;
        NN : not null access Integer := A'Access;
        procedure Add is new Increment (NN);
        procedure Forward (Value : access Integer) is
            generic
                Actual : access Integer;
            procedure Set_Value;
            procedure Set_Value is
            begin
                Actual.all := Actual.all + 5;
            end Set_Value;
            procedure Set_It is new Set_Value (Value);
        begin
            Set_It;
        end Forward;
    begin
        Instance.Update;
        Add;
        Forward (P);
        if A /= 5 or B /= 14 or Instance.Read /= 24 or Get_Default /= 29 then
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
    Ada.Text_IO.Put_Line ("GenericAnonymousObjects passed");
end GenericAnonymousObjects;
