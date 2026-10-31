with System;
procedure AbstractUseErrors is
    package Model is
        type Root is abstract tagged null record;
        procedure Act (Self : Root) is abstract;
        function Make return Root is abstract;
        type Opaque is abstract tagged private;
    private
        type Opaque is tagged null record;
    end Model;
    Private_Object : Model.Opaque;
    type Callback is access procedure (Self : Model.Root);
    Pointer : Callback := Model.Act'Access;
    Bad_Pointer : Callback := Model.Act'Unchecked_Access;
    Location : System.Address := Model.Act'Address;
    Object : Model.Root;
    type Reference is access Model.Root;
    Heap : Reference := new Model.Root;
    Aggregate : Model.Root := (null record);
    procedure Assign (Left : in out Model.Root; Right : Model.Root) is
    begin
        Left := Right;
    end Assign;
    generic
        type Item is private;
    package Type_Formal is
    end Type_Formal;
    package Type_Instance is new Type_Formal (Model.Root);
    generic
        Object : Model.Root;
    package Invalid_Formal is
    end Invalid_Formal;
    generic
        with procedure Action (Self : Model.Root);
    procedure Invoke;
    procedure Invoke is
    begin
        null;
    end Invoke;
    procedure Instance is new Invoke (Model.Act);
begin
    null;
end AbstractUseErrors;
