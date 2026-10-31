with Ada.Exceptions;
with System;
package body Occurrencepackage is
    procedure Observe (Item : Ada.Exceptions.Exception_Occurrence; Expected : System.Address) is
        Info : String := Ada.Exceptions.Exception_Information (Item);
        Marker : constant String := "occurrencepackage (body elaboration)";
        Found : Boolean := False;
    begin
        for I in Info'First .. Info'Last - Marker'Length + 1 loop
            if Info (I .. I + Marker'Length - 1) = Marker then
                Found := True;
            end if;
        end loop;
        Handled := Item'Address = Expected
            and Ada.Exceptions.Exception_Message (Item) = "library handler" and Found;
    end Observe;
begin
    raise Constraint_Error with "library handler";
exception
    when E : others => Observe (E, E'Address);
end Occurrencepackage;
