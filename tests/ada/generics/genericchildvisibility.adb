with Generic_Family;
with Child_Parent;
procedure GenericChildVisibility is
    package Hidden_Child is new Child_Parent.Child;
begin
    null;
end GenericChildVisibility;
