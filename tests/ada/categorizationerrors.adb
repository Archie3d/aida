package CategorizationErrors is
    pragma Pure;
    Variable : Integer := 0;
    type Pointer is access Integer;
    function Compute return Integer;
    Dynamic : constant Integer := Compute;
    type Record_Value is record
        Value : Integer := Compute;
    end record;
    Bad_Default : constant Record_Value := (Value => <>);
end CategorizationErrors;

package body CategorizationErrors is
    function Compute return Integer is
    begin
        return 1;
    end Compute;
    State : Integer := 0;
begin
    State := 1;
end CategorizationErrors;
