with Subunit_Dep; use Subunit_Dep;
separate (Subunits)
package body Nested is
    function Add (Value : Integer) return Integer is separate;
begin
    State := State + 1;
end Nested;
