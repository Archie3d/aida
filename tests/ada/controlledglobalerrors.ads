with Ada.Finalization;
package ControlledGlobalErrors is
    type Guard is new Ada.Finalization.Controlled with null record;
    Object : Guard;
end ControlledGlobalErrors;
