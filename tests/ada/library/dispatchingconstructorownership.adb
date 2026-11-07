with Ada.Tags.Generic_Dispatching_Constructor;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
with Ada.Text_IO;
with System;
procedure DispatchingConstructorOwnership is
    use type System.Address;
    Live : Integer := 0;
    type Parameters is limited record
        Value : Integer;
    end record;
    package Model is
        type Root is abstract new Ada.Finalization.Limited_Controlled with null record;
        function Make (Params : not null access Parameters) return Root is abstract;
        type Child is new Root with record
            Item : Integer := 0;
            Original : System.Address;
        end record;
        overriding procedure Initialize (Object : in out Child);
        overriding procedure Finalize (Object : in out Child);
        overriding function Make (Params : not null access Parameters) return Child;
    end Model;
    package body Model is
        procedure Initialize (Object : in out Child) is
        begin
            Live := Live + 1;
            Object.Original := Object'Address;
        end Initialize;
        procedure Finalize (Object : in out Child) is
        begin
            if Object.Original /= Object'Address then
                raise Program_Error;
            end if;
            Live := Live - 1;
        end Finalize;
        function Make (Params : not null access Parameters) return Child is
        begin
            return Result : Child do
                Result.Item := Params.Value;
                if Params.Value < 0 then
                    raise Constraint_Error;
                end if;
            end return;
        end Make;
    end Model;
    function Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.Root, Parameters, Model.Make);
    P : aliased Parameters := (Value => 42);
    Evaluations : Integer := 0;
    function Argument return not null access Parameters is
    begin
        Evaluations := Evaluations + 1;
        return P'Access;
    end Argument;
    function Selected_Tag return Ada.Tags.Tag is
    begin
        Evaluations := Evaluations + 1;
        return Model.Child'Tag;
    end Selected_Tag;
    function Forward return Model.Root'Class is
    begin
        return Create (Selected_Tag, Argument);
    end Forward;
    type Pointer is access Model.Root'Class;
    procedure Free is new Ada.Unchecked_Deallocation (Model.Root'Class, Pointer);
    Heap : Pointer;
begin
    declare
        X : Model.Root'Class := Forward;
    begin
        if Live /= 1 or Model.Child (X).Item /= 42 then
            raise Program_Error;
        end if;
    end;
    if Live /= 0 then raise Program_Error; end if;
    Heap := new Model.Root'Class'(Create (Selected_Tag, Argument));
    if Live /= 1 then raise Program_Error; end if;
    Free (Heap);
    if Live /= 0 then raise Program_Error; end if;
    P.Value := -1;
    begin
        declare
            X : Model.Root'Class := Create (Selected_Tag, Argument);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    if Live /= 0 or Evaluations /= 6 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("constructor ownership passed");
end DispatchingConstructorOwnership;
