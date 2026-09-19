with Ada.Exceptions;
with System;
package body Occurrencepackage is
    procedure Observe (Item : Ada.Exceptions.Exception_Occurrence; Expected : System.Address) is
    begin
        Handled := Item'Address = Expected
            and Ada.Exceptions.Exception_Message (Item) = "library handler";
    end Observe;
begin
    raise Constraint_Error with "library handler";
exception
    when E : others => Observe (E, E'Address);
end Occurrencepackage;
