with Ada.Text_IO;
package body TerminationObserver is
    protected body Log is
        procedure Note (Cause : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence) is
        begin
            if T /= Current_Task or else Is_Terminated (T) or else Is_Callable (T) then Bad := True; end if;
            if Attributes.Value (T) /= 11 then Bad := True; end if;
            Attributes.Set_Value (12, T);
            case Cause is
                when Normal => Normals := Normals + 1;
                when Abnormal => Abnormals := Abnormals + 1;
                when Unhandled_Exception =>
                    Exceptions := Exceptions + 1;
                    Save_Occurrence (Saved, X);
                    if Exception_Identity (X) /= Constraint_Error'Identity or else Exception_Message (X) /= "escaping" then Bad := True; end if;
            end case;
            if Cause /= Unhandled_Exception and then Exception_Identity (X) /= Null_Id then Bad := True; end if;
            -- Exceptions from a handler must not escape to the creator.
            raise Storage_Error with "ignored handler exception";
        end Note;
        procedure Specific (Cause : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence) is
        begin
            if Cause /= Normal or else not Finalized or else T /= Current_Task
                or else Exception_Identity (X) /= Null_Id then Bad := True; end if;
            Selected := Selected + 1;
        end Specific;
        procedure Environment (Cause : Cause_Of_Termination; T : Task_Id; X : Exception_Occurrence) is
        begin
            if Cause /= Normal or else T /= Current_Task or else Exception_Identity (X) /= Null_Id then raise Program_Error; end if;
            Ada.Text_IO.Put_Line ("environment termination OK");
        end Environment;
        function Count (Cause : Cause_Of_Termination) return Integer is
        begin
            case Cause is
                when Normal => return Normals;
                when Abnormal => return Abnormals;
                when Unhandled_Exception => return Exceptions;
            end case;
        end Count;
        function Specials return Integer is begin return Selected; end Specials;
        function Failed return Boolean is begin return Bad; end Failed;
    end Log;
end TerminationObserver;
