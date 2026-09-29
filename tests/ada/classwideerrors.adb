procedure ClasswideErrors is
    package P is
        type Root is tagged null record;
        procedure Pair (Left, Right : Root);
        type Child is new Root with null record;
    end P;
    package body P is
        procedure Pair (Left, Right : Root) is
        begin
            null;
        end Pair;
    end P;
    use P;
    package Late is
        type T is tagged null record;
        type D is new T with null record;
        procedure Added (Item : T);
    end Late;
    A : Root'Class;
    type Bad_Array is array (1 .. 2) of Root'Class;
    type Bad_Record is record
        Item : Root'Class;
    end record;
    type Bad_Parent is new Root'Class with null record;
    function Bad_Result return Root'Class;
    type Bad_Class is access Integer'Class;
    procedure Bad_Calls (Left : in out Root'Class; Right : Root) is
        Same : Boolean;
    begin
        Pair (Left, Right);
        Left := Right;
        Same := Left = Left;
    end Bad_Calls;
begin
    null;
end ClasswideErrors;
