with Ada.Exceptions;
with System;
package body Occurrencepackage is
    procedure Observe (Item : Ada.Exceptions.Exception_Occurrence; Expected : System.Address) is
    begin
        Handled := Item'Address = Expected;
    end Observe;
begin
    raise Constraint_Error;
exception
    when E : others => Observe (E, E'Address);
end Occurrencepackage;
