with Ep_Leaf;
package body Ep_Model is
    function Read return Integer is
    begin
        return Ep_Leaf.Read + 1;
    end Read;
end Ep_Model;
