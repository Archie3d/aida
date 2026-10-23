package RuntimeComponentLateErrors is
    N : Integer := 3;
    type Hidden is private;
    type Outer is record
        Value : Hidden;
        Tail : Integer;
    end record;
private
    type Hidden is record
        Data : String (1 .. N);
    end record;
end RuntimeComponentLateErrors;
