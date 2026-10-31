procedure Hello is

    type Node;
    type Node is record
        Value : Integer;
        Next : access all Node;
    end record;
begin
end Hello;
