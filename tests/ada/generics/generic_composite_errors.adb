procedure Generic_Composite_Errors is
    type Vector is array (Integer range <>) of Integer;
    type Pair is record
        Value : Integer;
    end record;
    generic
        Value : Vector;
    procedure Bad_Array;
    procedure Bad_Array is
    begin
        Value (Value'First) := 0;
        Value (Value'First .. Value'Last) := (others => 0);
    end Bad_Array;
    generic
        Value : Pair;
    procedure Bad_Record;
    procedure Bad_Record is
    begin
        Value.Value := 0;
    end Bad_Record;
    generic
        Value : in out Vector;
    package Reference is
    end Reference;
    Fixed : constant Vector := (1, 2, 3);
    function Make return Vector is
    begin
        return Fixed;
    end Make;
    package Constant_Actual is new Reference (Fixed);
    package Constant_Slice is new Reference (Fixed (1 .. 2));
    package Temporary_Actual is new Reference (Make);
    package Aggregate_Actual is new Reference ((1, 2, 3));
    package Hidden is
        type Item is limited private;
    private
        type Item is record
            Value : Integer;
        end record;
    end Hidden;
    generic
        Value : Hidden.Item;
    package Copy_Limited is
    end Copy_Limited;
    Limited_Value : Hidden.Item;
    package Limited_Copy is new Copy_Limited (Limited_Value);
begin
    null;
end Generic_Composite_Errors;
