with Categorized_Generic;
with Categorized_Types;
procedure PreelaborableActualErrors is
    package Invalid is new Categorized_Generic (Categorized_Types.Opaque);
begin
    null;
end PreelaborableActualErrors;
