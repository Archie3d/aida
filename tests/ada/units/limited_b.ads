limited with Limited_A;
package Limited_B is
    type A_Access is access all Limited_A.Node;
    Length : Integer := 3;
    type Vector is array (1 .. Length) of Integer;
    type Node is tagged record
        Value : Integer := 2;
        Peer : A_Access;
    end record;
    function Read (Object : Node) return Integer;
    type Derived_Node is new Node with null record;
    package Nested is
        type Scalar is range 1 .. 20;
        type Data is record
            Value : Integer := 9;
        end record;
    end Nested;
end Limited_B;
