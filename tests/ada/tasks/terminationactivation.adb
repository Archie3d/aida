with Ada.Text_IO;
with Ada.Task_Termination; use Ada.Task_Termination;
with Ada.Task_Identification; use Ada.Task_Identification;
with TerminationObserver; use TerminationObserver;
procedure TerminationActivation is
    Handled : Boolean := False;
    function Fail return Integer is
    begin
        raise Constraint_Error with "escaping";
        return 0;
    end Fail;
    procedure Subprogram_Activation is
        task Child;
        task body Child is
            X : Integer := Fail;
        begin raise Program_Error; end Child;
    begin raise Program_Error;
    exception when Tasking_Error => Handled := True;
    end Subprogram_Activation;
begin
    Set_Dependents_Fallback_Handler (Log.Note'Access);
    -- Activation belongs to the handled sequence, unlike declaration errors.
    declare
        task Child;
        task body Child is
            X : Integer := Fail;
        begin raise Program_Error; end Child;
    begin raise Program_Error;
    exception when Tasking_Error => Handled := True;
    end;
    if not Handled or else Log.Count (Unhandled_Exception) /= 1 then raise Program_Error; end if;
    Handled := False;
    Subprogram_Activation;
    if not Handled or else Log.Count (Unhandled_Exception) /= 2 then raise Program_Error; end if;
    begin
        declare
            task Unactivated;
            task body Unactivated is begin raise Program_Error; end Unactivated;
            X : Integer := Fail;
        begin raise Program_Error; end;
    exception when Constraint_Error => null; end;
    declare
        task Parent;
        task body Parent is
            task Child;
            task body Child is
                X : Integer := Fail;
            begin raise Program_Error; end Child;
        begin raise Program_Error;
        exception when Tasking_Error => null;
        end Parent;
    begin null; end;
    -- The parent completed its own activation before activating the child.
    -- Its handled activation failure must not fail the environment's activation.
    if Log.Count (Normal) /= 1 or else Log.Count (Unhandled_Exception) /= 3 then raise Program_Error; end if;
    if Log.Count (Abnormal) /= 1 or else Log.Failed then raise Program_Error; end if;
    Set_Dependents_Fallback_Handler (null);
    Ada.Text_IO.Put_Line ("termination during activation OK");
end TerminationActivation;
