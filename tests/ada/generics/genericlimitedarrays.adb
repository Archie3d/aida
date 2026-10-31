with Ada.Text_IO;
with Return_Model; use Return_Model;
procedure GenericLimitedArrays is
    procedure Check is
        generic
            Value : Cells;
        package Model is
            function Valid return Boolean;
        end Model;
        package body Model is
            function Valid return Boolean is
            begin
                for I in Value'Range loop
                    if Value (I).Self.all'Address /= Value (I).Value'Address then
                        return False;
                    end if;
                end loop;
                return Value'Length = 3;
            end Valid;
        end Model;
        package From_Call is new Model (Many (3));
        package From_Aggregate is new Model ((Make (1), Forward (2), Make (3)));
        subtype Three is Cells (8 .. 10);
        generic
            Value : Three;
        function Sliding return Boolean;
        function Sliding return Boolean is
        begin
            return Value'First = 8 and Value (8).Value = 3
                and Value (8).Self.all'Address = Value (8).Value'Address
                and Value (10).Self.all'Address = Value (10).Value'Address;
        end Sliding;
        function Test_Sliding is new Sliding (Many (3));
        type Matrix is array (1 .. 2, 1 .. 2) of Cell;
        generic
            Value : Matrix;
        function Matrix_Valid return Boolean;
        function Matrix_Valid return Boolean is
        begin
            return Value (1, 2).Self.all'Address = Value (1, 2).Value'Address
                and Value (2, 1).Value = 3;
        end Matrix_Valid;
        function Test_Matrix is new Matrix_Valid
            (((Make (1), Make (2)), (Make (3), Make (4))));
        generic
            Value : Buffer;
        function Buffer_Valid return Boolean;
        function Buffer_Valid return Boolean is
        begin
            return Value.Length = 4 and Value.Text = "zzzz"
                and Value.Item.Self.all'Address = Value.Item.Value'Address;
        end Buffer_Valid;
        function Test_Buffer is new Buffer_Valid (Make_Buffer (4));
    begin
        if not From_Call.Valid or not From_Aggregate.Valid or not Test_Sliding
            or not Test_Matrix or not Test_Buffer then
            raise Program_Error;
        end if;
        begin
            declare
                package Wrong_Length is new Model (Many (2));
                function Bad is new Sliding (Many (2));
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
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
    Ada.Text_IO.Put_Line ("GenericLimitedArrays passed");
end GenericLimitedArrays;
