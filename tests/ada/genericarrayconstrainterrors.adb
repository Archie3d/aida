procedure GenericArrayConstraintErrors is
    function Identity (Value : Integer) return Integer is
    begin
        return Value;
    end Identity;
    Limit : Integer := Identity (3);
    subtype Index is Integer range 1 .. Limit;
    subtype Other_Index is Integer range 1 .. Limit;
    subtype Component is Integer range 1 .. Identity (9);
    subtype Other_Component is Integer range 1 .. Identity (9);
    generic
        type A is array (Index) of Component;
    package Views is end Views;
    type Wrong_Index is array (Other_Index) of Component;
    type New_Evaluation is array (1 .. Limit) of Component;
    type Wrong_Component is array (Index) of Other_Component;
    type Open_Array is array (Index range <>) of Component;
    type Static_Array is array (1 .. 3) of Component;
    package Independent_Index is new Views (Wrong_Index);
    package Evaluated_Range is new Views (New_Evaluation);
    package Independent_Component is new Views (Wrong_Component);
    package Wrong_Category is new Views (Open_Array);
    package Coincident_Static is new Views (Static_Array);
    subtype Fresh_Constraint is Open_Array (1 .. Limit);
    package Independent_Range is new Views (Fresh_Constraint);
    generic
        type A is array (Index range <>) of Component;
    package Open_Views is end Open_Views;
    type Wrong_Open_Index is array (Other_Index range <>) of Component;
    package Independent_Open_Index is new Open_Views (Wrong_Open_Index);
    generic
        type A is array (Index, Index) of Component;
    package Matrices is end Matrices;
    type Wrong_Second is array (Index, Other_Index) of Component;
    package Wrong_Axis is new Matrices (Wrong_Second);
    subtype Chunk is String (1 .. Limit);
    subtype Other_Chunk is String (1 .. Limit);
    generic
        type A is array (Index) of Chunk;
    package Components is end Components;
    type Wrong_Chunks is array (Index) of Other_Chunk;
    package Independent_Composite is new Components (Wrong_Chunks);
begin
    null;
end GenericArrayConstraintErrors;
