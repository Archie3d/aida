with Ada.Text_IO;
with GenericArrayConstraintLibrary;
procedure GenericArrayConstraintLibrary_Main is
    function Identity (Value : Integer) return Integer is
    begin
        return Value;
    end Identity;
    procedure Check (Low, High : Integer) is
        subtype Index is Integer range Low .. High;
        subtype Alias_Index is Index;
        subtype Component is Integer range Identity (1) .. Identity (9);
        type Vector is array (Alias_Index) of Component;
        type Open_Vector is array (Alias_Index range <>) of Component;
        package Views is new GenericArrayConstraintLibrary (Index, Component, Vector, Open_Vector);
        type Second_Vector is array (Index) of Component;
        package Nested is new Views.Inner (Second_Vector);
        subtype Fixed_Vector is Open_Vector (Alias_Index);
        package Fixed_Views is new GenericArrayConstraintLibrary (Index, Component, Fixed_Vector, Open_Vector);
        package Fixed_Nested is new Fixed_Views.Inner (Vector);
        Value : Vector := (others => 4);
        Open_Value : Open_Vector (Low .. High) := (others => 5);
        Other : Second_Vector := (others => 6);
    begin
        if Fixed_Views.Copy (Fixed_Vector'(others => 7)) /= Fixed_Vector'(others => 7)
            or Fixed_Nested.Copy (Value) /= Value or Views.Copy (Value) /= Value or Views.Copy_Open (Open_Value) /= Open_Value
            or Nested.Copy (Other) /= Other then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check (2, 5);
        Check (7, 6);
    end Worker;
begin
    Check (-3, -1);
    Check (1, 0);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("library nonstatic array constraints passed");
end GenericArrayConstraintLibrary_Main;
