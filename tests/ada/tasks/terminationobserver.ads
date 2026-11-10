with Ada.Task_Attributes;
with Ada.Task_Termination; use Ada.Task_Termination;
with Ada.Task_Identification; use Ada.Task_Identification;
with Ada.Exceptions; use Ada.Exceptions;
package TerminationObserver is
    Saved : Exception_Occurrence;
    package Attributes is new Ada.Task_Attributes (Integer, 11);
    Finalized : Boolean := False;
    pragma Atomic (Finalized);
    protected Log is
        procedure Note (Cause : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence);
        procedure Specific (Cause : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence);
        procedure Environment (Cause : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence);
        function Count (Cause : Cause_Of_Termination) return Integer;
        function Specials return Integer;
        function Failed return Boolean;
    private
        Normals, Abnormals, Exceptions, Selected : Integer := 0;
        Bad : Boolean := False;
    end Log;
end TerminationObserver;
