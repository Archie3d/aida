with Ada.Text_IO;
with Ada.Finalization;
with Ada.Exceptions; use Ada.Exceptions;
with Ada.Task_Termination; use Ada.Task_Termination;
with Ada.Task_Identification; use Ada.Task_Identification;
with TerminationObserver; use TerminationObserver;
procedure TaskTermination is
    H : Termination_Handler := Log.Note'Access;
    type Guard is new Ada.Finalization.Limited_Controlled with null record;
    overriding procedure Finalize (Object : in out Guard) is
    begin Finalized := True; end Finalize;
    procedure Reject_Short_Lifetime is
        protected Local is
            procedure Note (C : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence);
        end Local;
        protected body Local is
            procedure Note (C : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence) is
            begin raise Constraint_Error; end Note;
        end Local;
    begin
        H := Local.Note'Access;
        raise Constraint_Error;
    exception when Program_Error => null;
    end Reject_Short_Lifetime;
begin
    Reject_Short_Lifetime;
    if Current_Task_Fallback_Handler /= null then raise Program_Error; end if;
    Set_Dependents_Fallback_Handler (H);
    if Current_Task_Fallback_Handler /= H then raise Program_Error; end if;
    declare
        task type Worker (Mode : Integer) is entry Start; end Worker;
        task body Worker is
            G : Guard;
        begin
            if Current_Task_Fallback_Handler /= null then raise Program_Error; end if;
            accept Start;
            case Mode is
                when 1 => null;
                when 2 => raise Constraint_Error with "escaping";
                when 3 => loop delay 60.0; end loop;
                when others =>
                    begin raise Constraint_Error; exception when Constraint_Error => null; end;
            end case;
        end Worker;
        A : Worker (1);
        B : Worker (2);
        C : Worker (3);
        D : Worker (4);
        E : Worker (1);
    begin
        if Specific_Handler (A'Identity) /= null then raise Program_Error; end if;
        Set_Specific_Handler (A'Identity, Log.Specific'Access);
        if Specific_Handler (A'Identity) /= Log.Specific'Access then raise Program_Error; end if;
        Set_Specific_Handler (E'Identity, Log.Specific'Access);
        Set_Specific_Handler (E'Identity, null);
        A.Start; B.Start; C.Start; D.Start; E.Start;
        abort C;
        while not A'Terminated loop delay 0.0; end loop;
        begin Set_Specific_Handler (A'Identity, H); raise Constraint_Error;
        exception when Tasking_Error => null; end;
        begin H := Specific_Handler (A'Identity); raise Constraint_Error;
        exception when Tasking_Error => null; end;
    end;
    begin Set_Specific_Handler (Null_Task_Id, null); raise Constraint_Error;
    exception when Program_Error => null; end;
    begin H := Specific_Handler (Null_Task_Id); raise Constraint_Error;
    exception when Program_Error => null; end;
    if Log.Count (Normal) /= 2 or else Log.Count (Abnormal) /= 1 or else Log.Count (Unhandled_Exception) /= 1
        or else Log.Specials /= 1 or else Log.Failed then raise Program_Error; end if;
    if Exception_Message (Saved) /= "escaping" then raise Program_Error; end if;
    Set_Dependents_Fallback_Handler (null);
    Ada.Text_IO.Put_Line ("task termination OK");
end TaskTermination;
