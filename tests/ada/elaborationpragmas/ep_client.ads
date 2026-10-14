with Ep_Model;
pragma Elaborate_All (Ep_Model);
package Ep_Client is
    Value : Integer := Ep_Model.Read;
end Ep_Client;
