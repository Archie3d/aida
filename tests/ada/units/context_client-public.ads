private with Context_Client.Hidden;
package Context_Client.Public is
    function Read return Integer;
private
    Value : Context_Secret.Data;
end Context_Client.Public;
