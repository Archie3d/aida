with Task_Failure_Model;
procedure TaskActivationFailure is
begin
    raise Program_Error with "main must not run";
end TaskActivationFailure;
