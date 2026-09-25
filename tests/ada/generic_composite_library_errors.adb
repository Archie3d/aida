package Dynamic_Source is
    type Vector is array (Integer range <>) of Integer;
    function Make return Vector;
end Dynamic_Source;
package body Dynamic_Source is
    function Make return Vector is
    begin
        return (1, 2, 3);
    end Make;
end Dynamic_Source;
generic
    Value : Dynamic_Source.Vector;
package Dynamic_Template is
end Dynamic_Template;
package Unsupported is new Dynamic_Template (Dynamic_Source.Make);
