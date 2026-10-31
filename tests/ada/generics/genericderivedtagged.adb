with Ada.Text_IO;
with Ada.Tags;
procedure GenericDerivedTagged is
    use type Ada.Tags.Tag;
    package Model is
        type Root is tagged record
            Value : Integer := 1;
        end record;
        function Make (Value : Integer := 2) return Root;
        function Read (Object : Root) return Integer;
        type Child is new Root with private;
    private
        type Child is new Root with record
            Extra : Integer := 10;
        end record;
        overriding function Make (Hidden_Name : Integer := 3) return Child;
        overriding function Read (Hidden_Object : Child) return Integer;
    end Model;
    package body Model is
        function Make (Value : Integer := 2) return Root is
        begin
            return (Value => Value);
        end Make;
        function Read (Object : Root) return Integer is
        begin
            return Object.Value;
        end Read;
        function Make (Hidden_Name : Integer := 3) return Child is
        begin
            return (Root'(Value => Hidden_Name) with Extra => 10);
        end Make;
        function Read (Hidden_Object : Child) return Integer is
        begin
            return Hidden_Object.Value + Hidden_Object.Extra;
        end Read;
    end Model;
    generic
        type T is new Model.Root with private;
    package Views is
        function Construct return T;
        function Score (Object : T) return Integer;
        function Dynamic_Score (Object : T'Class) return Integer;
        type Callback is access function (Object : T) return Integer;
        function Invoke (Object : T) return Integer;
        type Further is new T with null record;
    end Views;
    package body Views is
        function Construct return T is
        begin
            return Make (Value => 7);
        end Construct;
        function Score (Object : T) return Integer is
        begin
            return Read (Object => Object);
        end Score;
        function Dynamic_Score (Object : T'Class) return Integer is
        begin
            return Read (Object);
        end Dynamic_Score;
        function Invoke (Object : T) return Integer is
            Call : Callback := Read'Access;
        begin
            return Call (Object);
        end Invoke;
    end Views;
    package Child_View is new Views (Model.Child);
    package Root_View is new Views (Model.Root);
    generic
        type T is tagged private;
    function Tag_Of (Object : T) return Ada.Tags.Tag;
    function Tag_Of (Object : T) return Ada.Tags.Tag is
    begin
        return Object'Tag;
    end Tag_Of;
    function Child_Tag is new Tag_Of (Model.Child);
    generic
        type T (<>) is abstract tagged limited private;
    function Abstract_Tag (Object : T'Class) return Ada.Tags.Tag;
    function Abstract_Tag (Object : T'Class) return Ada.Tags.Tag is
    begin
        return Object'Tag;
    end Abstract_Tag;
    function Root_Tag is new Abstract_Tag (Model.Root);
    package Abstract_Model is
        type Root is abstract tagged null record;
        procedure Read (Object : Root) is abstract;
        type Concrete is new Root with null record;
        overriding procedure Read (Object : Concrete) is null;
    end Abstract_Model;
    function Concrete_Tag is new Abstract_Tag (Abstract_Model.Root);
    generic
        type T is new Abstract_Model.Root with private;
    procedure Read_Concrete (Object : T);
    procedure Read_Concrete (Object : T) is
    begin
        Read (Object);
    end Read_Concrete;
    procedure Read_Actual is new Read_Concrete (Abstract_Model.Concrete);
    procedure Check is
        Concrete : Abstract_Model.Concrete;
        A : Model.Child := Model.Make (Value => 4);
        B : Model.Child := Model.Make;
        C : Model.Child := Child_View.Construct;
        D : Child_View.Further := Child_View.Make (7);
        E : Model.Root := Root_View.Construct;
        type Callback is access function (Object : Model.Child) return Integer;
        Call : Callback := Model.Read'Access;
    begin
        Read_Actual (Concrete);
        if Concrete_Tag (Concrete) /= Abstract_Model.Concrete'Tag then
            raise Program_Error;
        end if;
        if Model.Read (Object => A) /= 14 or Model.Read (B) /= 12
            or Child_View.Score (C) /= 17 or Child_View.Dynamic_Score (C) /= 17
            or Child_View.Read (D) /= 17 or Child_View.Invoke (C) /= 17 or Root_View.Score (E) /= 7 or Call (A) /= 14
            or A'Tag /= Model.Child'Tag or D'Tag /= Child_View.Further'Tag
            or Child_Tag (C) /= Model.Child'Tag or Root_Tag (C) /= Model.Child'Tag then
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
    Ada.Text_IO.Put_Line ("generic tagged derived types passed");
end GenericDerivedTagged;
