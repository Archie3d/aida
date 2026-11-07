with Ada.Tags.Generic_Dispatching_Constructor;
package Constructor_Library is
    type Parameters is record
        Value : Integer;
    end record;
    type Root is tagged record
        Value : Integer;
    end record;
    function Make (Params : not null access Parameters) return Root;
    function Self_Make (Params : not null access Root) return Root;
    type Child is new Root with null record;
    overriding function Self_Make (Params : not null access Child) return Child;
    function Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Root, Parameters, Make);
end Constructor_Library;
package body Constructor_Library is
    function Self_Make (Params : not null access Root) return Root is
    begin
        return (Value => Params.Value);
    end Self_Make;
    function Self_Make (Params : not null access Child) return Child is
    begin
        return (Value => Params.Value + 10);
    end Self_Make;
    function Make (Params : not null access Parameters) return Root is
    begin
        return (Value => Params.Value);
    end Make;
end Constructor_Library;
with Ada.Tags.Generic_Dispatching_Constructor;
with Constructor_Library;
with Ada.Tags;
with Ada.Text_IO;
procedure DispatchingConstructorLibrary is
    package Model renames Constructor_Library;
    use type Ada.Tags.Tag;
    P : aliased Model.Parameters := (Value => 31);
    function Self_Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.Root, Model.Root, Model.Self_Make);
    Source : aliased Model.Root := (Value => 19);
    type Local is new Model.Root with null record;
begin
    declare
        X : Model.Root'Class := Model.Create (Model.Root'Tag, P'Access);
    begin
        if X'Tag /= Model.Root'Tag or X.Value /= 31 then raise Program_Error; end if;
    end;
    begin
        declare
            X : Model.Root'Class := Model.Create (Local'Tag, P'Access);
        begin
            raise Program_Error;
        end;
    exception
        when Ada.Tags.Tag_Error => null;
    end;
    declare
        X : Model.Root'Class := Self_Create (Model.Child'Tag, Source'Access);
    begin
        if X'Tag /= Model.Child'Tag or X.Value /= 29 then raise Program_Error; end if;
    end;
    Ada.Text_IO.Put_Line ("library constructors passed");
end DispatchingConstructorLibrary;
