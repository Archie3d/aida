with Limited_B;
package body Limited_A is
    procedure Link (Object : in out Node; Peer : B_Access) is
    begin
        Object.Peer := Peer;
        Object.Peer.Value := Object.Value + 10;
    end Link;
    function Inspect (Object : Limited_B.Node) return Integer is
    begin
        return Object.Read;
    end Inspect;
    function Create return access Limited_B.Node is
    begin
        return new Limited_B.Node;
    end Create;
end Limited_A;
