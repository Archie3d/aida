-- Representation conversion is lowered by the compiler for supported actuals.
generic
    type Source (<>) is limited private;
    type Target (<>) is limited private;
function Ada.Unchecked_Conversion (S : Source) return Target;
pragma Pure (Ada.Unchecked_Conversion);
