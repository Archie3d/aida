with Ada.Containers.Vectors;
with Ada.Strings.Unbounded;
procedure LibraryCollectionErrors is
    package Indefinite is new Ada.Containers.Vectors (Positive, String);
    type Limited_Item is limited record
        Value : Integer;
    end record;
    package Limited_Elements is new Ada.Containers.Vectors (Positive, Limited_Item);
    Text : Ada.Strings.Unbounded.Unbounded_String;
begin
    Text.Value.Data := null;
end LibraryCollectionErrors;
