with Generic_Family.Child;
procedure GenericChildDirectErrors is
    package Bad is new Generic_Family.Child;
    generic package Bad_Alias renames Generic_Family.Child;
begin
    null;
end GenericChildDirectErrors;
