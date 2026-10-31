private with Context_Secret;
pragma Elaborate (Context_Secret);
package Context_Client is
    type Data is private;
    function Read (Object : Data) return Integer;
    package Nested is
        function Read return Integer;
    private
        Value : Context_Secret.Data;
    end Nested;
private
    type Data is record
        Value : Context_Secret.Data;
    end record;
end Context_Client;
