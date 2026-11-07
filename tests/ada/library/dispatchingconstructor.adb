with Ada.Tags.Generic_Dispatching_Constructor;
with Ada.Text_IO;
procedure DispatchingConstructor is
    use type Ada.Tags.Tag;
    type Parameters is record
        Value : Integer;
    end record;
    type Saved_Access is access all Parameters;
    Saved : Saved_Access;
    package Model is
        type Root is abstract tagged private;
        function Make (Params : not null access Parameters) return Root is abstract;
        function Value (Object : Root) return Integer;
        type Child is new Root with private;
        overriding function Make (Params : not null access Parameters) return Child;
        function Extra (Object : Child) return Integer;
    private
        type Root is abstract tagged record
            Item : Integer;
        end record;
        type Child is new Root with record
            More : Integer;
        end record;
    end Model;
    Failure : exception;
    package body Model is
        function Value (Object : Root) return Integer is
        begin
            return Object.Item;
        end Value;
        function Extra (Object : Child) return Integer is
        begin
            return Object.More;
        end Extra;
        function Make (Params : not null access Parameters) return Child is
        begin
            if Params.Value = 77 then
                Saved := Saved_Access (Params);
            end if;
            if Params.Value < 0 then
                raise Failure;
            end if;
            return (Params.Value, Params.Value + 1);
        end Make;
    end Model;
    function Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.Root, Parameters, Model.Make);
    function Alias_Create (The_Tag : Ada.Tags.Tag;
        Params : not null access Parameters) return Model.Root'Class renames Create;
    generic
        with function Factory (The_Tag : Ada.Tags.Tag;
            Params : not null access Parameters) return Model.Root'Class;
    function Through (The_Tag : Ada.Tags.Tag;
        Params : not null access Parameters) return Model.Root'Class;
    function Through (The_Tag : Ada.Tags.Tag;
        Params : not null access Parameters) return Model.Root'Class is
    begin
        return Factory (The_Tag, Params);
    end Through;
    function Forwarded is new Through (Alias_Create);
    type Other is tagged null record;
    procedure Check is
        P : aliased Parameters := (Value => 42);
        Calls : Integer := 0;
        function Argument return not null access Parameters is
        begin
            Calls := Calls + 1;
            return P'Access;
        end Argument;
        procedure Reject (Tag : Ada.Tags.Tag) is
        begin
            declare
                X : Model.Root'Class := Create (Tag, P'Access);
            begin
                raise Program_Error;
            end;
        exception
            when Ada.Tags.Tag_Error => null;
        end Reject;
    begin
        declare
            X : Model.Root'Class := Forwarded (Params => Argument, The_Tag => Model.Child'Tag);
        begin
            if X'Tag /= Model.Child'Tag or Model.Value (X) /= 42
                or Model.Extra (Model.Child (X)) /= 43 or Calls /= 1 then
                raise Program_Error;
            end if;
        end;
        Reject (Ada.Tags.No_Tag);
        Reject (Model.Root'Tag);
        Reject (Other'Tag);
        declare
            type Deeper is new Model.Child with null record;
            function Local_Create is new Ada.Tags.Generic_Dispatching_Constructor
                (Model.Root, Parameters, Model.Make);
        begin
            Reject (Deeper'Tag);
            declare
                X : Model.Root'Class := Local_Create (Deeper'Tag, P'Access);
            begin
                if X'Tag /= Deeper'Tag or Model.Value (X) /= 42 then
                    raise Program_Error;
                end if;
            end;
        end;
        begin
            declare
                X : Model.Root'Class := Create (Model.Child'Tag, null);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        P.Value := 77;
        begin
            declare
                X : Model.Root'Class := Create (Model.Child'Tag, P'Access);
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => null;
        end;
        P.Value := -1;
        begin
            declare
                X : Model.Root'Class := Create (Model.Child'Tag, P'Access);
            begin
                raise Program_Error;
            end;
        exception
            when Failure => null;
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
    Ada.Text_IO.Put_Line ("dispatching constructors passed");
end DispatchingConstructor;
