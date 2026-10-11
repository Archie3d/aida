procedure GeneralAccessErrors is
    type Node;
    type Node_Access is access all Node;
    type Node_Constant is access constant Node;
    type Node is record
        Value : Integer;
        Next : Node_Access;
    end record;
begin
    null;
end GeneralAccessErrors;
