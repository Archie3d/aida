with Return_Model; use Return_Model;
procedure GenericLimitedErrors is
    Object : Cell := Make (1);
    type Pointer is access all Cell;
    P : Pointer := new Cell'(Make (2));
    Values : Cells := Many (2);
    Record_Value : Pair := Make_Pair;
    generic
        Value : Cell;
    package Model is end Model;
    package Bad_Name is new Model (Object);
    package Bad_Qualified is new Model (Cell'(Object));
    package Bad_Dereference is new Model (P.all);
    package Bad_Index is new Model (Values (3));
    package Bad_Component is new Model (Record_Value.First);
    generic
        Value : Cell := Object;
    package Bad_Default is end Bad_Default;
    generic
        Value : Cells;
    package Arrays is end Arrays;
    package Bad_Array is new Arrays (Values);
    package Bad_Slice is new Arrays (Values (3 .. 4));
    generic
        Value : Pair;
    package Records is end Records;
    package Bad_Record is new Records (Record_Value);
    package Bad_Aggregate is new Records ((First => Object, Second => Make (3)));
    generic
        type T is limited private;
        Value : T;
        Copy : T := Value;
    package Bad_Contract is end Bad_Contract;
    generic
        type T is limited private;
        type Vector is array (Integer range <>) of T;
        Seed : T;
        Value : Vector := (1 => Seed);
    package Bad_Component_Default is end Bad_Component_Default;
    generic
        Value : Cell;
    procedure Assign;
    procedure Assign is
    begin
        Value.Value := 9;
    end Assign;
    generic
        Value : in out Cell;
    package References is end References;
    package Bad_Reference is new References (Make (1));
    task type Task_Object;
    task body Task_Object is
    begin
        null;
    end Task_Object;
    type Tasks is array (1 .. 2) of Task_Object;
    generic
        Value : Tasks;
    package Unsupported is end Unsupported;
    package Bad_Tasks is new Unsupported ((others => <>));
begin
    null;
end GenericLimitedErrors;
