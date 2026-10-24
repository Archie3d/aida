limited with Limited_B;
package Limited_A is
    package Renamed_B renames Limited_B;
    package Renamed_Nested renames Limited_B.Nested;
    type B_Access is access all Limited_B.Node;
    type Scalar_Access is access Limited_B.Nested.Scalar;
    type Nested_Access is access Limited_B.Nested.Data;
    type B_Class_Access is access all Limited_B.Node'Class;
    type Derived_Access is access all Limited_B.Derived_Node'Class;
    subtype Incomplete_B is Limited_B.Node;
    subtype Vector_View is Limited_B.Vector;
    type Node is tagged record
        Value : Integer := 1;
        Peer : B_Access;
    end record;
    procedure Link (Object : in out Node; Peer : B_Access);
    function Inspect (Object : Limited_B.Node) return Integer;
    function Create return access Limited_B.Node;
end Limited_A;
