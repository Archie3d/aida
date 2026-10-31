with Renaming_View;
with Renaming_Chain;
procedure RenamingVisibilityErrors is
    function Good is new Renaming_Chain;
    function Hidden is new Renaming_Source;
begin
    Renaming_Model.Increase;
end RenamingVisibilityErrors;
