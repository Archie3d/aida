with Ada.Text_IO;
procedure GenericArrayConstraints is
    function Identity (Value : Integer) return Integer is
    begin
        return Value;
    end Identity;
    procedure Check (Low, High : Integer) is
        Calls : Integer := 0;
        function Bound (Value : Integer) return Integer is
        begin
            Calls := Calls + 1;
            return Value;
        end Bound;
        subtype Index is Integer range Bound (Low) .. Bound (High);
        subtype Alias_Index is Index;
        subtype Component is Integer range Identity (1) .. Identity (9);
        subtype Alias_Component is Component;
        type Vector is array (Alias_Index) of Alias_Component;
        subtype Alias_Vector is Vector;
        generic
            type A is array (Index) of Component;
        package Views is
            function Total (Value : A) return Integer;
            function Make return A;
        end Views;
        package body Views is
            function Total (Value : A) return Integer is
                Result : Integer := 0;
            begin
                for I in Value'Range loop
                    Result := Result + Value (I);
                end loop;
                return Result;
            end Total;
            function Make return A is
                Value : A := (others => 3);
            begin
                return Value;
            end Make;
        end Views;
        package View is new Views (Alias_Vector);
        type Open_Vector is array (Index range <>) of Component;
        subtype Fixed_Vector is Open_Vector (Alias_Index);
        package Fixed_View is new Views (Fixed_Vector);
        type Derived_Vector is new Vector;
        package Derived_View is new Views (Derived_Vector);
        subtype Columns is Integer range 1 .. 2;
        type Matrix is array (Alias_Index, 1 .. 2) of Component;
        generic
            type A is array (Index, Columns) of Component;
        function Copy (Value : A) return A;
        function Copy (Value : A) return A is
        begin
            return Value;
        end Copy;
        function Copy_Matrix is new Copy (Matrix);
        M : Matrix := (others => (others => 4));
        Value : Vector := View.Make;
    begin
        if Calls /= 2 or M /= Copy_Matrix (M)
            or Fixed_View.Total (Fixed_View.Make) /= 3 * Value'Length
            or Derived_View.Total (Derived_View.Make) /= 3 * Value'Length
            or Value'First /= Low or Value'Last /= High
            or View.Total (Value) /= 3 * Value'Length then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check (2, 5);
        Check (3, 2);
    end Worker;
begin
    Check (-2, 1);
    Check (7, 6);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("nonstatic formal array constraints passed");
end GenericArrayConstraints;
