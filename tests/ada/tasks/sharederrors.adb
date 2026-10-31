procedure SharedErrors is
    type Record_Type is record
        value : Integer;
    end record;
    pragma Atomic (Record_Type);
    type Array_Type is array (1 .. 2) of Integer;
    pragma Volatile (Array_Type);
    text : String (1 .. 2) := "ab";
    pragma Atomic (text);
    constantValue : constant Integer := 1;
    pragma Volatile (constantValue);
    value : Integer := 0;
    alias : Integer renames value;
    pragma Atomic (alias);
    subtype Sub is Integer range 0 .. 10;
    pragma Atomic (Sub);
    pragma Atomic (missing);
    type Incomplete;
    pragma Atomic (Incomplete);
    type Late is new Integer;
    lateObject : Late := 0;
    pragma Atomic (Late);
    type Parent is new Integer;
    type Child is new Parent;
    childObject : constant Child := 1;
    pragma Volatile (Parent);
    procedure Invalid (parameter : in out Integer) is
        pragma Atomic (parameter);
    begin
        null;
    end Invalid;
begin
    null;
end SharedErrors;
