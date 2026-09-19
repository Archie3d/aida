with System;

-- Occurrence bindings currently retain exception identity. Inspection and
-- message operations will extend this representation in a later phase.
package Ada.Exceptions is
    type Exception_Occurrence is limited private;
private
    type Exception_Occurrence is record
        Identity : System.Address;
    end record;
end Ada.Exceptions;
