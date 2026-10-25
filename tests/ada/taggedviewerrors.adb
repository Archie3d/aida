procedure TaggedViewErrors is
    type Defaulted (N : Integer := 1) is tagged null record;
    type Base (N : Positive) is tagged null record;
    type Unconstrained_Parent (M : Positive) is new Base with null record;
    type Tiny is tagged null record;
    for Tiny'Size use 8;
    package Model is
        type Root is tagged record
            Public_Value : Integer;
        end record;
        type Private_Extension is new Root with private;
        type Hidden (<>) is private;
        type Tagged_Hidden (<>) is tagged private;
        function Make return Hidden;
        type Known (Count : Positive) is tagged private;
    private
        type Private_Extension is new Root with record
            Secret : Integer;
        end record;
        type Hidden (Length : Positive) is record
            Data : String (1 .. Length);
        end record;
        type Known (Count : Positive) is tagged null record;
        type Tagged_Hidden is tagged null record;
    end Model;
    package body Model is
        function Make return Hidden is
        begin
            return (Length => 1, Data => "x");
        end Make;
    end Model;
    type Opaque_Child is new Model.Tagged_Hidden with null record;
    Missing_Child : Opaque_Child;
    subtype Bad_Child is Opaque_Child (1);
    Item : Model.Private_Extension;
    Unknown : Model.Hidden := Model.Make;
    Missing : Model.Hidden;
    type Missing_Array is array (1 .. 2) of Model.Hidden;
    subtype Bad_Constraint is Model.Hidden (1);
    type Link is access Model.Hidden;
    Pointer : Link := new Model.Hidden;
    Secret : Integer := Item.Secret;
    Discriminant : Integer := Unknown.Length;
    type Plain;
    type Plain_Link is access all Plain;
    type Plain is record
        Self : Plain_Link := Plain'Access;
    end record;
    package Wrong_Completion is
        type Root is tagged null record;
        type Other is tagged null record;
        type Child is new Root with private;
    private
        type Child is new Other with null record;
    end Wrong_Completion;
begin
    null;
end TaggedViewErrors;
